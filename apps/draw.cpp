/**
 *  Copyright 2015 Mike Reed
 */

#include "GWindow.h"
#include "../include/GBitmap.h"
#include "../include/GCanvas.h"
#include "../include/GColor.h"
#include "../include/GRandom.h"
#include "../include/GRect.h"
#include "../include/GShader.h"

#include <vector>

static const float CORNER_SIZE = 9;

template <typename T> int find_index(const std::vector<T*>& list, T* target) {
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == target) {
            return i;
        }
    }
    return -1;
}

static GRandom gRand;

static GColor rand_color() {
    return {gRand.nextF(), gRand.nextF(), gRand.nextF(), 1};
}

static GRect make_from_pts(const GPoint& p0, const GPoint& p1) {
    return GRect::LTRB(std::min(p0.x, p1.x), std::min(p0.y, p1.y),
                       std::max(p0.x, p1.x), std::max(p0.y, p1.y));
}

static bool contains(const GRect& rect, float x, float y) {
    return rect.left < x && x < rect.right && rect.top < y && y < rect.bottom;
}

static bool hit_test(float x0, float y0, float x1, float y1) {
    const float dx = fabs(x1 - x0);
    const float dy = fabs(y1 - y0);
    return std::max(dx, dy) <= CORNER_SIZE;
}

static bool in_resize_corner(const GRect& r, float x, float y, GPoint* anchor) {
    if (hit_test(r.left, r.top, x, y)) {
        *anchor = {r.right, r.bottom};
        return true;
    } else if (hit_test(r.right, r.top, x, y)) {
        *anchor = {r.left, r.bottom};
        return true;
    } else if (hit_test(r.right, r.bottom, x, y)) {
        *anchor = {r.left, r.top};
        return true;
    } else if (hit_test(r.left, r.bottom, x, y)) {
        *anchor = {r.right, r.top};
        return true;
    }
    return false;
}

static void draw_corner(GCanvas* canvas, const GColor& c, float x, float y, float dx, float dy) {
    canvas->fillRect(make_from_pts({x, y - 1}, {x + dx, y + 1}), c);
    canvas->fillRect(make_from_pts({x - 1, y}, {x + 1, y + dy}), c);
}

static void constrain_color(GColor* c) {
    c->a = std::max(std::min(c->a, 1.f), 0.1f);
    c->r = std::max(std::min(c->r, 1.f), 0.f);
    c->g = std::max(std::min(c->g, 1.f), 0.f);
    c->b = std::max(std::min(c->b, 1.f), 0.f);
}

static void draw_point(GCanvas* canvas, GPoint p) {
    canvas->drawRect(GRect::LTRB(p.x - 2, p.y - 2, p.x + 3, p.y + 3), GPaint());
}

static void draw_line(GCanvas* canvas, GPoint p0, GPoint p1, GColor c, float width) {
    GVector norm = { p1.y - p0.y, p0.x - p1.x };
    float scale = width / 2 / norm.length();
    norm.x *= scale;
    norm.y *= scale;

    GPoint quad[4] = { p0 + norm, p1 + norm, p1 - norm, p0 - norm };
    canvas->drawConvexPolygon(quad, 4, GPaint(c));
}


class Shape {
public:
    Shape() {
        fGradPts[0] = { 10, 10 };
        fGradPts[1] = { 100, 80 };
    }

    virtual ~Shape() {}
    virtual GRect getRect() = 0;
    virtual void setRect(const GRect&) {}
    virtual bool animates() const { return false; }

    GColor getColor() {
        if (fGradient) {
            return fColors[0];
        } else {
            return this->onGetColor();
        }
    }

    void setColor(const GColor& c) {
        if (fGradient) {
            for (int i = 0; i < fColors.size(); ++i) {
                fColors[i].a = c.a;
            }
            this->rebuildGradient();
        } else {
            this->onSetColor(c);
        }
    }

    void draw(GCanvas* canvas) {
        GPaint paint;
        this->updatePaint(&paint);
        this->onDraw(canvas, paint);
    }

    virtual void drawHilite(GCanvas* canvas) {
        this->drawBoundsHilite(canvas);
        this->drawGradientHilite(canvas);
    }

    void toggleGradient() {
        if (fGradient) {
            fGradient = nullptr;
        } else {
            int n = (gRand.nextU() % 3) + 2;
            fColors.clear();
            for (int i = 0; i < n; ++i) {
                fColors.push_back(rand_color());
            }

            GRect r = this->getRect();
            fGradPts[0] = { r.left + 10, r.top + 10 };
            fGradPts[1] = { r.right - 10, r.bottom - 10 };
            this->rebuildGradient();
        }
    }

    void offset(float dx, float dy) {
        this->setRect(this->getRect().offset(dx, dy));

        if (fGradient) {
            GMatrix::Translate(dx, dy).mapPoints(fGradPts, 2);
            this->rebuildGradient();
        }
    }

    void rebuildGradient() {
        fGradient = GCreateLinearGradient(fGradPts[0], fGradPts[1],
                                          fColors.data(), fColors.size());
    }

    virtual GClick* findClick(GPoint p, GWindow* wind) {
        if (fGradient) {
            int index = -1;
            for (int i = 0; i < 2; ++i) {
                if (hit_test(p.x, p.y, fGradPts[i].x, fGradPts[i].y)) {
                    index = i;
                }
            }
            if (index >= 0) {
                return new GClick(p, [this, wind, index](GClick* click) {
                    fGradPts[index] = click->curr();
                    this->rebuildGradient();
                    wind->requestDraw();
                });
            }
        }
        return nullptr;
    }

protected:
    virtual void onDraw(GCanvas* canvas, const GPaint&) {}
    virtual GColor onGetColor() = 0;
    virtual void onSetColor(const GColor&) {}

    void updatePaint(GPaint* paint) {
        paint->setColor(this->getColor());
        if (fGradient) {
            paint->setShader(fGradient);
        }
    }

    void drawBoundsHilite(GCanvas* canvas) {
        GRect r = this->getRect();
        const float size = CORNER_SIZE;
        GColor c = {0, 0, 0, 1};
        draw_corner(canvas, c, r.left, r.top, size, size);
        draw_corner(canvas, c, r.left, r.bottom, size, -size);
        draw_corner(canvas, c, r.right, r.top, -size, size);
        draw_corner(canvas, c, r.right, r.bottom, -size, -size);
    }

    void drawGradientHilite(GCanvas* canvas) {
        if (fGradient) {
            draw_line(canvas, fGradPts[0], fGradPts[1], {0,0,0,1}, 1.4f);
            draw_point(canvas, fGradPts[0]);
            draw_point(canvas, fGradPts[1]);
        }
    }

    GPoint fGradPts[2];
    std::vector<GColor> fColors;
    std::shared_ptr<GShader> fGradient;
};

#include "draw_path.cpp"

class RectShape : public Shape {
public:
    RectShape(GColor c) : fColor(c) {
        fRect = GRect::XYWH(0, 0, 0, 0);
    }

    void onDraw(GCanvas* canvas, const GPaint& paint) override {
        canvas->drawRect(fRect, paint);
    }

    GRect getRect() override { return fRect; }
    void setRect(const GRect& r) override { fRect = r; }
    GColor onGetColor() override { return fColor; }
    void onSetColor(const GColor& c) override { fColor = c; }

private:
    GRect   fRect;
    GColor  fColor;
};

class BitmapShape : public Shape {
public:
    BitmapShape(const GBitmap& bm) : fBM(bm) {
        fRect = GRect::XYWH(20, 20, 150, 150);
    }

    void onDraw(GCanvas* canvas, const GPaint&) override {
        GPaint paint;
#if 0
        GMatrix inv, m = GMatrix::Scale(fRect.width() / fBM.width(),
                                        fRect.height() / fBM.height());
        m.postTranslate(fRect.left, fRect.top);
        if (m.invert(&inv)) {
            auto sh = GCreateBitmapShader(fBM, inv);
            paint.setShader(sh.get());
            canvas->drawRect(fRect, paint);
        }
#else
        paint.setShader(GCreateBitmapShader(fBM, GMatrix()));

        canvas->save();
        canvas->translate(fRect.left, fRect.top);
        canvas->scale(fRect.width() / fBM.width(),
                      fRect.height() / fBM.height());
        canvas->drawRect(GRect::WH(fBM.width(), fBM.height()), paint);
        canvas->restore();
#endif
    }

    GRect getRect() override { return fRect; }
    void setRect(const GRect& r) override { fRect = r; }
    GColor onGetColor() override { return fColor; }
    void onSetColor(const GColor& c) override { fColor = c; }

private:
    GRect   fRect;
    GColor  fColor = { 1, 0, 0, 0 };
    GBitmap fBM;
};

static void make_regular_poly(GPoint pts[], int count, float cx, float cy, float rx, float ry) {
    float angle = 0;
    const float deltaAngle = gFloatPI * 2 / count;

    for (int i = 0; i < count; ++i) {
        pts[i] = {cx + cos(angle) * rx, cy + sin(angle) * ry};
        angle += deltaAngle;
    }
}

class ConvexShape : public Shape {
public:
    ConvexShape(GColor c, int sides) : fPaint(c), fN(sides) {
        fBounds = GRect::XYWH(100, 100, 150, 150);
    }

    void onDraw(GCanvas* canvas, const GPaint& paint) override {
        float sx = fBounds.width() * 0.5f;
        float sy = fBounds.height() * 0.5f;
        float cx = (fBounds.left + fBounds.right) * 0.5f;
        float cy = (fBounds.top + fBounds.bottom) * 0.5f;

        GPoint* pts = new GPoint[fN];
        make_regular_poly(pts, fN, cx, cy, sx, sy);
        fPaint.setShader(paint.shareShader());
        canvas->drawConvexPolygon(pts, fN, fPaint);
        delete[] pts;
    }
    
    GRect getRect() override { return fBounds; }
    void setRect(const GRect& r) override { fBounds = r; }
    GColor onGetColor() override { return fPaint.getColor(); }
    void onSetColor(const GColor& c) override { fPaint.setColor(c); }

private:
    GPaint  fPaint;
    int     fN;
    GRect   fBounds;
};

static Shape* cons_up_shape(unsigned index) {
    const char* names[] = {
        "apps/spock.png", "apps/wheel.png",
    };
    if (index < GARRAY_COUNT(names)) {
        GBitmap bm;
        if (bm.readFromFile(names[index])) {
            return new BitmapShape(bm);
        }
    }
    if (index == 2) {
        int n = (int)(3 + gRand.nextF() * 12);
        return new ConvexShape(rand_color(), n);
    }
    if (index == 3) {
        return new PolyShape({0, 0.75, 1, 1}, 2);
    }
    if (index == 4) {
        return new PolyShape({1, 0, 0.5, 1}, 1);
    }
    return nullptr;
}

class TestWindow : public GWindow {
    std::vector<Shape*> fList;
    Shape* fShape;
    GColor fBGColor;
    bool fShowAnimation = false;

public:
    TestWindow(int w, int h) : GWindow(w, h) {
        fBGColor = {1, 1, 1, 1};
        fShape = NULL;
    }

    virtual ~TestWindow() {}
    
protected:
    void onDraw(GCanvas* canvas) override {
        canvas->fillRect(GRect::XYWH(0, 0, 10000, 10000), fBGColor);

        for (int i = 0; i < fList.size(); ++i) {
            fList[i]->draw(canvas);
        }
        if (fShape) {
            fShape->drawHilite(canvas);
            if (fShowAnimation && fShape->animates()) {
                this->requestDraw();
            }
        }
    }

    bool onKeyPress(uint32_t sym) override {
        if (Shape* s = cons_up_shape(sym - '1')) {
            fList.push_back(fShape = s);
            this->updateTitle();
            this->requestDraw();
            return true;
        }

        if (fShape) {
            switch (sym) {
                case SDLK_UP: {
                    int index = find_index(fList, fShape);
                    if (index < fList.size() - 1) {
                        std::swap(fList[index], fList[index + 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DOWN: {
                    int index = find_index(fList, fShape);
                    if (index > 0) {
                        std::swap(fList[index], fList[index - 1]);
                        this->requestDraw();
                        return true;
                    }
                    return false;
                }
                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    this->removeShape(fShape);
                    fShape = NULL;
                    this->updateTitle();
                    this->requestDraw();
                    return true;
                case 'l':
                    fShape->toggleGradient();
                    this->requestDraw();
                    return true;
                default:
                    break;
            }
        }

        GColor c = fShape ? fShape->getColor() : fBGColor;
        const float delta = 0.1f;
        switch (sym) {
            case 'a': c.a -= delta; break;
            case 'A': c.a += delta; break;
            case 'r': c.r -= delta; break;
            case 'R': c.r += delta; break;
            case 'g': c.g -= delta; break;
            case 'G': c.g += delta; break;
            case 'b': c.b -= delta; break;
            case 'B': c.b += delta; break;

            case ' ': fShowAnimation = !fShowAnimation; break;

            default: return false;
        }
        constrain_color(&c);
        if (fShape) {
            fShape->setColor(c);
        } else {
            c.a = 1;   // need the bg to stay opaque
            fBGColor = c;
        }
        this->updateTitle();
        this->requestDraw();
        return true;
    }

    GClick* onFindClickHandler(GPoint loc) override {
        if (fShape) {
            if (GClick* click = fShape->findClick(loc, this)) {
                return click;
            }
            GPoint anchor;
            if (in_resize_corner(fShape->getRect(), loc.x, loc.y, &anchor)) {
                return new GClick(loc, [this, anchor](GClick* click) {
                    fShape->setRect(make_from_pts(click->curr(), anchor));
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }

        for (int i = fList.size() - 1; i >= 0; --i) {
            if (contains(fList[i]->getRect(), loc.x, loc.y)) {
                fShape = fList[i];
                this->updateTitle();
                return new GClick(loc, [this](GClick* click) {
                    const GPoint curr = click->curr();
                    const GPoint prev = click->prev();
                    fShape->offset(curr.x - prev.x, curr.y - prev.y);
                    this->updateTitle();
                    this->requestDraw();
                });
            }
        }
        
        // else create a new shape
        fShape = new RectShape(rand_color());
        fList.push_back(fShape);
        this->updateTitle();
        return new GClick(loc, [this](GClick* click) {
            if (fShape && GClick::kUp_State == click->state()) {
                if (fShape->getRect().isEmpty()) {
                    this->removeShape(fShape);
                    fShape = NULL;
                    return;
                }
            }
            fShape->setRect(make_from_pts(click->orig(), click->curr()));
            this->updateTitle();
            this->requestDraw();
        });
    }

private:
    void removeShape(Shape* target) {
        assert(target);

        std::vector<Shape*>::iterator it = std::find(fList.begin(), fList.end(), target);
        if (it != fList.end()) {
            fList.erase(it);
        } else {
            assert(!"shape not found?");
        }
    }

    void updateTitle() {
        char buffer[100];
        buffer[0] = ' ';
        buffer[1] = 0;

        GColor c = fBGColor;
        if (fShape) {
            c = fShape->getColor();
        }

        snprintf(buffer, sizeof(buffer), "R:%02X  G:%02X  B:%02X  A:%02X",
                int(c.r * 255), int(c.g * 255), int(c.b * 255), int(c.a * 255));
        this->setTitle(buffer);
    }

    typedef GWindow INHERITED;
};

int main(int argc, char const* const* argv) {
    GWindow* wind = new TestWindow(640, 480);

    return wind->run();
}

