#include "engine/display/Shape.h"

#include "engine/display/BitmapData.h"

#include "engine/assets/Library.h"
#include "engine/render/Renderer.h"
#include "engine/render/ShapeCache.h"

namespace engine {

    Shape::Shape(Library& library, std::uint16_t characterId) : library_(library), id_(characterId) {}

    Rectangle Shape::localBounds() const { return Rectangle::fromSwf(library_.bounds(id_, ratio())); }

    bool Shape::hitTestLocal(Point local) const {
        if (!DisplayObject::hitTestLocal(local))
            return false; // cheap bounds check first

        const swf::DrawList* drawList = library_.drawList(id_, ratio());
        if (drawList == nullptr)
            return false;

        // Static text is hit anywhere in its bounds, like Flash.
        if (std::holds_alternative<swf::TextDef>(*library_.find(id_)))
            return true;

        return swf::hitTest(*drawList, static_cast<float>(local.x * swf::kTwipsPerPixel),
                            static_cast<float>(local.y * swf::kTwipsPerPixel));
    }

    void Shape::draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool /*asMask*/) {
        const Matrix toDevice = world.then(Matrix::scale(context.deviceScale, context.deviceScale));
        const ShapeCache::Entry* entry = context.shapes.get(id_, ratio(), toDevice);
        if (!entry)
            return;

        context.renderer.drawTexture(entry->texture, entry->textureToLocal.then(world), color);
    }

    Bitmap::Bitmap(Library& library, std::uint16_t bitmapId) : library_(&library), id_(bitmapId) {}
    Bitmap::Bitmap(std::shared_ptr<BitmapData> bitmapData) : bitmapData_(std::move(bitmapData)) {}
    Bitmap::~Bitmap() = default;

    void Bitmap::setBitmapData(std::shared_ptr<BitmapData> bitmapData) {
        bitmapData_ = std::move(bitmapData);
        uploadedVersion_ = 0;
    }

    const vector::Image* Bitmap::image() const {
        if (bitmapData_)
            return &bitmapData_->image();

        return library_ != nullptr ? library_->bitmap(id_) : nullptr;
    }

    Rectangle Bitmap::localBounds() const {
        const vector::Image* picture = image();
        return picture ? Rectangle{0, 0, static_cast<double>(picture->width), static_cast<double>(picture->height)}
                       : Rectangle{};
    }

    void Bitmap::draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool /*asMask*/) {
        const vector::Image* picture = image();
        if (picture == nullptr || picture->pixels.empty())
            return;

        if (bitmapData_) {
            // Upload the pixels again when the game changed them.
            if (!texture_ || texture_->width() != picture->width || texture_->height() != picture->height) {
                texture_ = std::make_unique<Texture>(*picture);
            } else if (uploadedVersion_ != bitmapData_->version()) {
                texture_->update(*picture);
            }

            uploadedVersion_ = bitmapData_->version();
        } else if (!texture_) {
            texture_ = std::make_unique<Texture>(*picture);
        }

        context.renderer.drawTexture(*texture_, world, color);
    }

} // namespace engine
