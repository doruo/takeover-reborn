// Shape.h - leaf objects drawn from library art: vector shapes, morph shapes,
// static texts (all rasterized through the ShapeCache) and bitmaps.
#pragma once

#include "engine/display/DisplayObject.h"

#include <memory>

namespace vector {
    struct Image;
}

namespace engine {

    class BitmapData;
    class Library;
    class Texture;

    /// A timeline vector character: DefineShape, DefineMorphShape (drawn at the
    /// placement's ratio) or DefineText.
    class Shape : public DisplayObject {
    public:
        Shape(Library& library, std::uint16_t characterId);

        [[nodiscard]] Rectangle localBounds() const override;
        [[nodiscard]] bool hitTestLocal(Point local) const override;

    protected:
        void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

    private:
        Library& library_;
        std::uint16_t id_;
    };

    /// flash.display.Bitmap: shows a library bitmap or a BitmapData.
    class Bitmap : public DisplayObject {
    public:
        Bitmap(Library& library, std::uint16_t bitmapId);
        explicit Bitmap(std::shared_ptr<BitmapData> bitmapData);
        ~Bitmap() override;

        [[nodiscard]] Rectangle localBounds() const override;
        [[nodiscard]] const std::shared_ptr<BitmapData>& bitmapData() const { return bitmapData_; }
        void setBitmapData(std::shared_ptr<BitmapData> bitmapData);

    protected:
        void draw(RenderContext& context, const Matrix& world, const ColorTransform& color, bool asMask) override;

    private:
        [[nodiscard]] const vector::Image* image() const;

        Library* library_ = nullptr;
        std::uint16_t id_ = 0;
        std::shared_ptr<BitmapData> bitmapData_;
        std::uint64_t uploadedVersion_ = 0; ///< Version of bitmapData_ in the texture.
        std::unique_ptr<Texture> texture_;  ///< Uploaded on first draw.
    };

} // namespace engine
