#ifndef GPath_DEFINED
#define GPath_DEFINED

#include "GMatrix.h"
#include "GPoint.h"
#include "GRect.h"

#include <vector>

enum GPathVerb {
    kMove,  // returns pts[0] from Iter
    kLine,  // returns pts[0]..pts[1] from Iter and Edger
};

enum class GPathDirection {
    kCW,  // clockwise
    kCCW, // counter-clockwise
};

class GPath : public std::enable_shared_from_this<GPath> {
public:
    /**
     *  Return the bounds of all of the control-points in the path.
     *
     *  If there are no points, returns an empty rect (all zeros)
     */
    GRect bounds() const;

    size_t countPoints() const { return fPts.size(); }

    /**
     *  Create a new path by transforming the points in this path.
     */
    std::shared_ptr<GPath> transform(const GMatrix&) const;

    std::shared_ptr<GPath> offset(float dx, float dy) const {
        return this->transform(GMatrix::Translate(dx, dy));
    }

    // maximum number of points returned by Iter::next() and Edger::next()
    enum {
        kMaxNextPoints = 2
    };

    /**
     *  Walks the path, returning each verb that was entered.
     *  e.g.    moveTo() returns kMove
     *          lineTo() returns kLine
     */
    class Iter {
    public:
        Iter(const GPath&);
        nonstd::optional<GPathVerb> next(GPoint pts[]);

    private:
        const GPoint*    fCurrPt;
        const GPathVerb* fCurrVb;
        const GPathVerb* fStopVb;
    };

    /**
     *  Walks the path, returning "edges" only. Thus it does not return kMove, but will return
     *  the final closing "edge" for each contour.
     *  e.g.
     *      path.moveTo(A).lineTo(B).lineTo(C).moveTo(D).lineTo(E)
     *  will return
     *      kLine   A..B
     *      kLine   B..C
     *      kLine   C..A
     *      kLine   D..E
     *      kLine   E..D
     *
     * Typical calling pattern...
     *
     *   GPoint pts[GPath::kMaxNextPoints];
     *   GPath::Edger edger(path);
     *   while (auto v = edger.next(pts)) {
     *       switch (v.value()) {
     *           case GPath::kLine:
     *  }
     */
    class Edger {
    public:
        Edger(const GPath&);
        nonstd::optional<GPathVerb> next(GPoint pts[]);

    private:
        const GPoint*    fPrevMove;
        const GPoint*    fCurrPt;
        const GPathVerb* fCurrVb;
        const GPathVerb* fStopVb;
        int fPrevVerb;
    };

    GPath(std::vector<GPoint> pts, std::vector<GPathVerb> vbs)
        : fPts(std::move(pts))
        , fVbs(std::move(vbs))
    {}

private:
    friend class GPathBuilder;

    const std::vector<GPoint>    fPts;
    const std::vector<GPathVerb> fVbs;
};

#endif

