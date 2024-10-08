#ifndef GPathBuilder_DEFINED
#define GPathBuilder_DEFINED

#include "GPoint.h"
#include "GRect.h"
#include "GPath.h"

#include <optional>
#include <vector>

class GPathBuilder {
public:
    GPathBuilder() {}

    /**
     *  Erase any previously added points/verbs, restoring the path to its initial empty state.
     */
    void reset();

    /**
     *  Start a new contour at the specified coordinate.
     *  Returns a reference to this path.
     */
    void moveTo(GPoint p) {
        fPts.push_back(p);
        fVbs.push_back(GPathVerb::kMove);
    }
    void moveTo(float x, float y) { this->moveTo({x, y}); }

    /**
     *  Connect the previous point (either from a moveTo or lineTo) with a line segment to
     *  the specified coordinate.
     *  Returns a reference to this path.
     */
    void lineTo(GPoint p) {
        assert(fVbs.size() > 0);
        fPts.push_back(p);
        fVbs.push_back(GPathVerb::kLine);
    }
    void lineTo(float x, float y) { this->lineTo({x, y}); }

    /**
     *  Append a new contour to this path, made up of the 4 points of the specified rect,
     *  in the specified direction.
     *
     *  In either direction the contour must begin at the top-left corner of the rect.
     */
    void addRect(const GRect&, GPathDirection = GPathDirection::kCW);

    /**
     *  Append a new contour to this path with the specified polygon.
     *  Calling this is equivalent to calling moveTo(pts[0]), lineTo(pts[1..count-1]).
     */
    void addPolygon(const GPoint pts[], int count);

    void transform(const GMatrix&);

    /**
     * Return a GPath from the contents of this builder,
     * and then reset() the builder back to its empty state.
     */
    std::shared_ptr<GPath> detach();

private:
    std::vector<GPoint>    fPts;
    std::vector<GPathVerb> fVbs;
};

#endif

