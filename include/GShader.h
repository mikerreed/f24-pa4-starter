/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GShader_DEFINED
#define GShader_DEFINED

#include <memory>
#include "GColor.h"
#include "GPixel.h"
#include "GPoint.h"

class GBitmap;
class GMatrix;

/**
 *  GShaders create colors to fill whatever geometry is being drawn to a GCanvas.
 */
class GShader : public std::enable_shared_from_this<GShader> {
public:
    virtual ~GShader() {}

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    virtual bool isOpaque() = 0;

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    virtual bool setContext(const GMatrix& ctm) = 0;

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    virtual void shadeRow(int x, int y, int count, GPixel row[]) = 0;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the subclass can not be created.
 */
std::shared_ptr<GShader> GCreateBitmapShader(const GBitmap&, const GMatrix& localMatrix);

/**
 *  Return a subclass of GShader that draws the specified gradient of [count] colors between
 *  the two points. Color[0] corresponds to p0, and Color[count-1] corresponds to p1, and all
 *  intermediate colors are evenly spaced between.
 *
 *  The gradient colors are GColors, and therefore unpremul. The output colors (in shadeRow)
 *  are GPixel, and therefore premul. The gradient has to interpolate between pairs of GColors
 *  before "pre" multiplying them into GPixels.
 *
 *  If count < 1, this should return nullptr.
 */
std::shared_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor[], int count);

static inline std::shared_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1,
                                                             const GColor& c0, const GColor& c1) {
    const GColor colors[] = { c0, c1 };
    return GCreateLinearGradient(p0, p1, colors, 2);
}
#endif
