#include "engine/display/DisplayObject.h"

#include "engine/display/DisplayObjectContainer.h"
#include "engine/display/Stage.h"
#include "engine/render/Renderer.h"

#include <algorithm>

#include <cmath>
#include <numbers>

namespace engine {

    // /----- HELPERS -----/

    namespace {

        /// Flash keeps positions in twips and truncates when converting from pixels.
        double toTwipPrecision(double pixels) {
            return std::trunc(pixels * swf::kTwipsPerPixel) / swf::kTwipsPerPixel;
        }

        /// Wraps an angle in degrees into (-180, 180].
        double normalizeDegrees(double degrees) {
            double d = std::fmod(degrees, 360.0);
            if (d > 180.0)
                d -= 360.0;
            else if (d <= -180.0)
                d += 360.0;

            return d;
        }

    } // namespace

    // /----- TRANSFORM -----/

    void DisplayObject::setMatrix(const Matrix& m) {
        matrix_ = m;
        decompositionValid_ = false;
    }

    void DisplayObject::setX(double value) { matrix_.tx = toTwipPrecision(value); }
    void DisplayObject::setY(double value) { matrix_.ty = toTwipPrecision(value); }

    void DisplayObject::cacheDecomposition() const {
        if (decompositionValid_)
            return;

        // Same decomposition as the Flash Player: two axis angles give rotation
        // and skew, the axis lengths give the scales.
        rotationRadians_ = std::atan2(matrix_.b, matrix_.a);
        skewRadians_ = std::atan2(-matrix_.c, matrix_.d) - rotationRadians_;
        scaleX_ = std::hypot(matrix_.a, matrix_.b);
        scaleY_ = std::hypot(matrix_.c, matrix_.d);
        decompositionValid_ = true;
    }

    void DisplayObject::rebuildMatrix() {
        const double rotationY = rotationRadians_ + skewRadians_;
        matrix_.a = scaleX_ * std::cos(rotationRadians_);
        matrix_.b = scaleX_ * std::sin(rotationRadians_);
        matrix_.c = -scaleY_ * std::sin(rotationY);
        matrix_.d = scaleY_ * std::cos(rotationY);
    }

    double DisplayObject::scaleX() const {
        cacheDecomposition();
        return scaleX_;
    }

    double DisplayObject::scaleY() const {
        cacheDecomposition();
        return scaleY_;
    }

    double DisplayObject::rotation() const {
        cacheDecomposition();
        return normalizeDegrees(rotationRadians_ * 180.0 / std::numbers::pi);
    }

    void DisplayObject::setScaleX(double value) {
        cacheDecomposition();
        scaleX_ = value;
        rebuildMatrix();
    }

    void DisplayObject::setScaleY(double value) {
        cacheDecomposition();
        scaleY_ = value;
        rebuildMatrix();
    }

    void DisplayObject::setRotation(double degrees) {
        cacheDecomposition();
        rotationRadians_ = normalizeDegrees(degrees) * std::numbers::pi / 180.0;
        rebuildMatrix();
    }

    void DisplayObject::setAlpha(double value) {
        // Flash stores the alpha multiplier as 8.8 fixed point: 0.33 reads back
        // as 0.328125.
        colorTransform_.alphaMultiplier = static_cast<float>(std::trunc(value * 256.0) / 256.0);
    }

    double DisplayObject::width() const { return getBounds(nullptr).width; }
    double DisplayObject::height() const { return getBounds(nullptr).height; }

    void DisplayObject::setWidth(double value) {
        // Flash: scaleX = value / (width at scale 1), rotation kept.
        const double unscaled = width() / (scaleX() == 0 ? 1 : std::abs(scaleX()));
        if (unscaled > 0)
            setScaleX(value / unscaled);
    }

    void DisplayObject::setHeight(double value) {
        const double unscaled = height() / (scaleY() == 0 ? 1 : std::abs(scaleY()));
        if (unscaled > 0)
            setScaleY(value / unscaled);
    }

    // /----- COORDINATES AND HIERARCHY -----/

    Matrix DisplayObject::concatenatedMatrix() const {
        Matrix m = matrix_;

        for (const DisplayObject* p = parent_; p != nullptr; p = p->parent_)
            m = m.then(p->matrix_);

        return m;
    }

    Rectangle DisplayObject::getBounds(const DisplayObject* targetSpace) const {
        const Rectangle local = localBounds();

        if (targetSpace == this)
            return local;

        Matrix toTarget;

        if (targetSpace == nullptr) {
            toTarget = matrix_;
        } else {
            toTarget = concatenatedMatrix().then(targetSpace->concatenatedMatrix().inverted());
        }

        return local.transformed(toTarget);
    }

    Stage* DisplayObject::stage() const {
        const DisplayObject* top = this;
        while (top->parent_ != nullptr)
            top = top->parent_;

        return dynamic_cast<Stage*>(const_cast<DisplayObject*>(top));
    }

    DisplayObject* DisplayObject::root() {
        DisplayObject* top = this;
        while (top->parent_ != nullptr)
            top = top->parent_;

        return top;
    }

    double DisplayObject::mouseX() const {
        const Stage* s = stage();
        return s ? globalToLocal({s->stageMouseX(), s->stageMouseY()}).x : 0;
    }

    double DisplayObject::mouseY() const {
        const Stage* s = stage();
        return s ? globalToLocal({s->stageMouseX(), s->stageMouseY()}).y : 0;
    }

    bool DisplayObject::hitTestPoint(double x, double y, bool shapeFlag) const {
        const Point local = globalToLocal({x, y});

        if (!shapeFlag) {
            const Rectangle b = localBounds();
            return local.x >= b.x && local.x <= b.right() && local.y >= b.y && local.y <= b.bottom();
        }

        return hitTestLocal(local);
    }

    bool DisplayObject::hitTestLocal(Point local) const {
        const Rectangle b = localBounds();
        return local.x >= b.x && local.x <= b.right() && local.y >= b.y && local.y <= b.bottom();
    }

    // /----- EVENTS -----/

    bool DisplayObject::dispatchEvent(Event& event) {
        // Keep the target and its ancestors alive: a listener may remove them
        // from the display list (e.g. a click that closes the screen).
        // The bubbling path is fixed before any listener runs, as in Flash.
        std::vector<DisplayObject*> path;
        std::vector<std::shared_ptr<DisplayObject>> keepAlive;

        for (DisplayObject* o = this; o != nullptr; o = o->parent_) {
            path.push_back(o);

            if (auto alive = o->weak_from_this().lock())
                keepAlive.push_back(std::move(alive));
        }

        event.target_ = this;
        notifyListeners(event);

        if (!event.bubbles())
            return true;

        for (std::size_t i = 1; i < path.size() && !event.isPropagationStopped(); ++i)
            path[i]->notifyListeners(event);

        return true;
    }

    // /----- TIMELINE PLACEMENT -----/

    void DisplayObject::applyPlacement(const swf::PlaceObject& place) {
        if (place.matrix)
            setMatrix(Matrix::fromSwf(*place.matrix));

        if (place.colorTransform)
            colorTransform_ = ColorTransform::fromSwf(*place.colorTransform);

        if (place.ratio)
            ratio_ = *place.ratio;

        if (place.name)
            name_ = *place.name;

        if (place.clipDepth)
            clipDepth_ = *place.clipDepth;

        if (place.filters)
            filters_ = *place.filters;

        if (place.blendMode)
            blendMode_ = *place.blendMode;

        if (place.visible)
            visible_ = *place.visible;
    }

    // /----- RENDERING -----/

    void DisplayObject::render(RenderContext& context, const Matrix& parentWorld, const ColorTransform& parentColor) {
        if (!visible_)
            return;

        renderWorld(context, matrix_.then(parentWorld), colorTransform_.then(parentColor));
    }

    void DisplayObject::renderWorld(RenderContext& context, const Matrix& world, const ColorTransform& color) {
        const bool needsLayer = !filters_.empty() || blendMode_ != swf::BlendMode::Normal;
        if (!needsLayer) {
            draw(context, world, color, false);
            return;
        }

        // Filters and blend modes work on an image of the whole object: draw it
        // offscreen, filter it, composite it (see Renderer). The color transform
        // is applied when compositing, so it also tints the glow or shadow.
        Renderer& renderer = context.renderer;
        const Rectangle local = localBounds();
        if (local.isEmpty())
            return;

        const Rectangle device = local.transformed(world.then(renderer.stageToTarget()));
        int padding = 1;

        for (const swf::Filter& filter : filters_)
            padding += renderer.filterPadding(filter);

        const PixelRect target = renderer.targetRect();
        const int x0 = std::max(static_cast<int>(std::floor(device.x)) - padding, target.x - padding);
        const int y0 = std::max(static_cast<int>(std::floor(device.y)) - padding, target.y - padding);
        const int x1 = std::min(static_cast<int>(std::ceil(device.right())) + padding, target.x + target.width + padding);
        const int y1 = std::min(static_cast<int>(std::ceil(device.bottom())) + padding, target.y + target.height + padding);
        if (x1 <= x0 || y1 <= y0)
            return;

        renderer.pushLayer({x0, y0, x1 - x0, y1 - y0});
        draw(context, world, ColorTransform{}, false);

        for (const swf::Filter& filter : filters_)
            renderer.applyFilter(filter);

        renderer.popLayer(blendMode_, color);
    }

    void DisplayObject::renderMask(RenderContext& context, const Matrix& parentWorld) {
        // A mask's own visibility doesn't matter in Flash: its shape always clips.
        draw(context, matrix_.then(parentWorld), ColorTransform{}, true);
    }

} // namespace engine
