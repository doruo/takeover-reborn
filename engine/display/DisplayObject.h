// DisplayObject.h - base of everything on the display list (flash.display).
//
// It is also an EventDispatcher: mouse events bubble from the object under
// the cursor up to the stage, as in Flash.
//
// Ownership model. Flash objects are garbage collected: game code may keep a
// reference to a clip after the timeline removed it, and keep using it. To
// reproduce that safely, display objects are held by std::shared_ptr: the
// parent owns its children, and any game object that keeps a reference shares
// that ownership. The parent link is a plain pointer, cleared on removal.
#pragma once

#include "engine/events/EventDispatcher.h"
#include "engine/geom/Matrix.h"
#include "swf/Timeline.h"
#include "swf/Types.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace engine {

    class DisplayObjectContainer;
    class Library;
    class Renderer;
    class ShapeCache;
    class Stage;

    /// What a render pass needs. Passed down the tree.
    struct RenderContext {
        Renderer& renderer;
        ShapeCache& shapes;
        double deviceScale = 1; ///< Framebuffer pixels per stage pixel.
    };

    class DisplayObject : public EventDispatcher, public std::enable_shared_from_this<DisplayObject> {
    public:
        DisplayObject() = default;
        ~DisplayObject() override = default;

        // /----- IDENTITY -----/

        [[nodiscard]] const std::string& name() const { return name_; }
        void setName(std::string name) { name_ = std::move(name); }
        [[nodiscard]] DisplayObjectContainer* parent() const { return parent_; }
        /// The stage this object is on, or nullptr when it isn't displayed.
        [[nodiscard]] Stage* stage() const;

        /// Topmost ancestor (the object itself if it has no parent).
        [[nodiscard]] DisplayObject* root();

        // /----- TRANSFORM (FLASH.DISPLAY.DISPLAYOBJECT PROPERTIES) -----/

        //
        // x/y are stored in twips like Flash: assigning 0.33 reads back 0.3.
        // scaleX/scaleY/rotation are cached separately from the matrix, as Flash
        // does, so that setting one doesn't make the others drift.

        [[nodiscard]] const Matrix& matrix() const { return matrix_; }
        void setMatrix(const Matrix& m);

        [[nodiscard]] double x() const { return matrix_.tx; }
        [[nodiscard]] double y() const { return matrix_.ty; }
        void setX(double value);
        void setY(double value);

        [[nodiscard]] double scaleX() const;
        [[nodiscard]] double scaleY() const;
        [[nodiscard]] double rotation() const; ///< Degrees, in (-180, 180].
        void setScaleX(double value);
        void setScaleY(double value);
        void setRotation(double degrees);

        [[nodiscard]] double alpha() const { return colorTransform_.alphaMultiplier; }
        void setAlpha(double value); ///< Stored as 8.8 fixed point, like Flash.

        [[nodiscard]] const ColorTransform& colorTransform() const { return colorTransform_; }
        void setColorTransform(const ColorTransform& cx) { colorTransform_ = cx; }

        [[nodiscard]] bool visible() const { return visible_; }
        void setVisible(bool visible) { visible_ = visible; }

        [[nodiscard]] swf::BlendMode blendMode() const { return blendMode_; }
        void setBlendMode(swf::BlendMode mode) { blendMode_ = mode; }

        [[nodiscard]] const std::vector<swf::Filter>& filters() const { return filters_; }
        void setFilters(std::vector<swf::Filter> filters) { filters_ = std::move(filters); }

        /// Size of the bounds in the parent's space.
        [[nodiscard]] double width() const;
        [[nodiscard]] double height() const;

        /// Scales the object so its bounds get this size (text fields resize
        /// their box instead).
        virtual void setWidth(double value);
        virtual void setHeight(double value);

        // /----- GEOMETRY -----/

        /// Bounds of this object's content in its own coordinate space.
        [[nodiscard]] virtual Rectangle localBounds() const { return {}; }

        /// Bounds expressed in `targetSpace` (nullptr = this object's parent).
        [[nodiscard]] Rectangle getBounds(const DisplayObject* targetSpace) const;

        /// Transform from this object's space to the root of its tree.
        [[nodiscard]] Matrix concatenatedMatrix() const;

        [[nodiscard]] Point localToGlobal(Point local) const { return concatenatedMatrix().apply(local); }
        [[nodiscard]] Point globalToLocal(Point global) const { return concatenatedMatrix().inverted().apply(global); }

        /// Mouse position in this object's space (needs to be on the stage).
        [[nodiscard]] double mouseX() const;
        [[nodiscard]] double mouseY() const;

        /// Does the point (in root/stage coordinates) touch this object? With
        /// `shapeFlag`, tests the actual vector outlines, else the bounding box.
        [[nodiscard]] bool hitTestPoint(double x, double y, bool shapeFlag = false) const;

        /// Exact hit test of a point in this object's own space, ignoring
        /// visibility (used by hitTestPoint and mouse picking).
        [[nodiscard]] virtual bool hitTestLocal(Point local) const;

        // /----- INTERACTION (FLASH.DISPLAY.INTERACTIVEOBJECT) -----/

        // Shapes and bitmaps are not interactive in Flash: the mouse targets their
        // parent. Sprites, clips, buttons and text fields are.

        [[nodiscard]] virtual bool isInteractive() const { return false; }
        [[nodiscard]] bool mouseEnabled() const { return mouseEnabled_; }
        void setMouseEnabled(bool enabled) { mouseEnabled_ = enabled; }

        /// Bubbles the event from this object up to the stage.
        bool dispatchEvent(Event& event) override;

        // /----- TIMELINE BOOKKEEPING -----/

        // Set when a timeline places the object. Used to replay frames exactly.

        [[nodiscard]] int depth() const { return depth_; }
        [[nodiscard]] std::uint16_t characterId() const { return characterId_; }
        [[nodiscard]] bool placedByTimeline() const { return placedByTimeline_; }
        [[nodiscard]] int placeFrame() const { return placeFrame_; }
        [[nodiscard]] std::optional<int> clipDepth() const { return clipDepth_; }
        [[nodiscard]] std::uint16_t ratio() const { return ratio_; }
        void setRatio(std::uint16_t ratio) { ratio_ = ratio; }

        /// Applies the properties carried by a PlaceObject record.
        void applyPlacement(const swf::PlaceObject& place);

        // /----- PER-FRAME HOOKS -----/

        /// Advances timelines by one frame (no-op for static objects).
        virtual void advanceFrame() {}

        /// Draws the object. `parentWorld` maps the parent's space to the stage.
        virtual void render(RenderContext& context, const Matrix& parentWorld, const ColorTransform& parentColor);

        /// Draws the object with `world` as its full transform, ignoring its own
        /// matrix and visibility (BitmapData.draw does that).
        void renderWorld(RenderContext& context, const Matrix& world, const ColorTransform& color);

        /// Draws only the object's coverage, for use as a mask.
        virtual void renderMask(RenderContext& context, const Matrix& parentWorld);

    protected:
        /// Subclasses draw themselves here. `world` already includes this object.
        virtual void draw(RenderContext&, const Matrix& /*world*/, const ColorTransform& /*color*/, bool /*asMask*/) {}

    private:
        friend class DisplayObjectContainer;
        friend class MovieClip;

        void cacheDecomposition() const;
        void rebuildMatrix();

        std::string name_;
        DisplayObjectContainer* parent_ = nullptr;
        bool mouseEnabled_ = true;

        Matrix matrix_;

        // Cached decomposition of matrix_ (see setScaleX & co).
        mutable bool decompositionValid_ = true;
        mutable double scaleX_ = 1, scaleY_ = 1, rotationRadians_ = 0, skewRadians_ = 0;

        ColorTransform colorTransform_;
        bool visible_ = true;
        swf::BlendMode blendMode_ = swf::BlendMode::Normal;
        std::vector<swf::Filter> filters_;

        int depth_ = 0;
        std::uint16_t characterId_ = 0;
        bool placedByTimeline_ = false;
        int placeFrame_ = 0;
        std::optional<int> clipDepth_;
        std::uint16_t ratio_ = 0;
    };

    using DisplayObjectPtr = std::shared_ptr<DisplayObject>;

} // namespace engine
