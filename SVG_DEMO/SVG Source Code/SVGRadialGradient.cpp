//#include "stdafx.h"
//#include "SVGRadialGradient.h"
//#include <algorithm>
//#include <vector>
//#include <cmath>
//#include <sstream>
//#include <string>
//
//using namespace std;
//
//// --- Helper Parser cho Transform (Xử lý rotate(a, x, y) mà parser cũ có thể thiếu) ---
//static void applyTransformString(Gdiplus::Matrix* matrix, const string& transStr) {
//    if (transStr.empty()) return;
//
//    matrix->Reset(); // Reset về Identity để parse lại từ đầu chính xác
//
//    string s = transStr;
//    // Thay thế các ký tự phân cách lạ bằng khoảng trắng để dễ parse
//    for (char& c : s) {
//        if (c == ',' || c == '(' || c == ')') c = ' ';
//    }
//
//    stringstream ss(s);
//    string token;
//
//    while (ss >> token) {
//        if (token == "matrix") {
//            float m[6];
//            for (int i = 0; i < 6; ++i) ss >> m[i];
//            Gdiplus::Matrix temp(m[0], m[1], m[2], m[3], m[4], m[5]);
//            matrix->Multiply(&temp, Gdiplus::MatrixOrderAppend);
//        }
//        else if (token == "translate") {
//            float tx, ty = 0;
//            ss >> tx;
//            if (!(ss >> ty)) ty = 0; // ty optional
//            matrix->Translate(tx, ty, Gdiplus::MatrixOrderAppend);
//        }
//        else if (token == "scale") {
//            float sx, sy;
//            ss >> sx;
//            if (!(ss >> sy)) sy = sx; // sy optional defaults to sx
//            matrix->Scale(sx, sy, Gdiplus::MatrixOrderAppend);
//        }
//        else if (token == "rotate") {
//            float angle, cx = 0, cy = 0;
//            ss >> angle;
//            // Kiểm tra xem có cx, cy không
//            streampos oldPos = ss.tellg();
//            if (ss >> cx >> cy) {
//                // Rotate(angle, cx, cy) = Translate(cx, cy) * Rotate(angle) * Translate(-cx, -cy)
//                matrix->Translate(cx, cy, Gdiplus::MatrixOrderAppend);
//                matrix->Rotate(angle, Gdiplus::MatrixOrderAppend);
//                matrix->Translate(-cx, -cy, Gdiplus::MatrixOrderAppend);
//            }
//            else {
//                ss.clear();
//                ss.seekg(oldPos); // Quay lại nếu không đọc được cx, cy
//                matrix->Rotate(angle, Gdiplus::MatrixOrderAppend);
//            }
//        }
//        else if (token == "skewX") {
//            float angle; ss >> angle;
//            matrix->Shear(tan(angle * 3.14159265f / 180.0f), 0, Gdiplus::MatrixOrderAppend);
//        }
//        else if (token == "skewY") {
//            float angle; ss >> angle;
//            matrix->Shear(0, tan(angle * 3.14159265f / 180.0f), Gdiplus::MatrixOrderAppend);
//        }
//    }
//}
//
//SVGRadialGradient::SVGRadialGradient() {
//    cx = 0.5f; cy = 0.5f; r = 0.5f;
//    fx = 0.5f; fy = 0.5f;
//}
//
//void SVGRadialGradient::parseAttributes(xml_node<>* node) {
//    SVGGradient::parseAttributes(node);
//
//    auto parseVal = [&](xml_attribute<>* attr, float defVal) {
//        if (!attr) return defVal;
//        string val = attr->value();
//        if (val.back() == '%') {
//            val.pop_back();
//            return (float)atof(val.c_str()) / 100.0f;
//        }
//        return (float)atof(val.c_str());
//        };
//
//    cx = parseVal(node->first_attribute("cx"), 0.5f);
//    cy = parseVal(node->first_attribute("cy"), 0.5f);
//    r = parseVal(node->first_attribute("r"), 0.5f);
//    fx = parseVal(node->first_attribute("fx"), cx);
//    fy = parseVal(node->first_attribute("fy"), cy);
//
//    // QUAN TRỌNG: Re-parse gradientTransform tại chỗ để đảm bảo hỗ trợ rotate(a, x, y)
//    // Base class có thể parse thiếu trường hợp này.
//    if (xml_attribute<>* attr = node->first_attribute("gradientTransform")) {
//        if (!this->gradientTransform) this->gradientTransform = new Gdiplus::Matrix();
//        applyTransformString(this->gradientTransform, attr->value());
//    }
//}
//
//static float distSq(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2) {
//    return (p1.X - p2.X) * (p1.X - p2.X) + (p1.Y - p2.Y) * (p1.Y - p2.Y);
//}
//
//Gdiplus::Brush* SVGRadialGradient::createBrush(const Gdiplus::RectF& boundingBox) {
//    if (stops.empty()) {
//        return new Gdiplus::SolidBrush(Gdiplus::Color::Black);
//    }
//
//    REAL rawCx = (REAL)cx;
//    REAL rawCy = (REAL)cy;
//    REAL rawR = (REAL)r;
//    REAL rawFx = (REAL)fx;
//    REAL rawFy = (REAL)fy;
//
//    // 1. Focal Point Clamping (Chuẩn SVG)
//    REAL dx = rawFx - rawCx;
//    REAL dy = rawFy - rawCy;
//    REAL dist = sqrt(dx * dx + dy * dy);
//    // Giữ tiêu cự nằm trong 99% bán kính để tránh lỗi GDI+
//    if (rawR > 0 && dist >= rawR * 0.99f) {
//        REAL scale = (rawR * 0.99f) / dist;
//        rawFx = rawCx + dx * scale;
//        rawFy = rawCy + dy * scale;
//    }
//
//    // 2. Build Final Matrix
//    Gdiplus::Matrix finalMatrix;
//    if (gradientTransform) {
//        finalMatrix.Multiply(gradientTransform);
//    }
//    if (gradientUnits == "objectBoundingBox") {
//        finalMatrix.Scale(boundingBox.Width, boundingBox.Height, Gdiplus::MatrixOrderAppend);
//        finalMatrix.Translate(boundingBox.X, boundingBox.Y, Gdiplus::MatrixOrderAppend);
//    }
//
//    // 3. Dynamic Expansion (Tự động mở rộng bán kính để không bị clipping)
//    REAL expansionScale = 1.0f;
//    Gdiplus::Matrix inverseMatrix;
//    inverseMatrix.Multiply(&finalMatrix);
//
//    if (inverseMatrix.Invert() == Gdiplus::Status::Ok) {
//        Gdiplus::PointF corners[4] = {
//            Gdiplus::PointF(boundingBox.X, boundingBox.Y),
//            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y),
//            Gdiplus::PointF(boundingBox.X, boundingBox.Y + boundingBox.Height),
//            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y + boundingBox.Height)
//        };
//        inverseMatrix.TransformPoints(corners, 4);
//
//        float maxDistSq = 0.0f;
//        Gdiplus::PointF centerP(rawCx, rawCy);
//        for (int i = 0; i < 4; i++) {
//            float d = distSq(centerP, corners[i]);
//            if (d > maxDistSq) maxDistSq = d;
//        }
//        float maxDist = sqrt(maxDistSq);
//
//        if (rawR > 0 && maxDist > rawR) {
//            expansionScale = (maxDist / rawR) * 1.01f; // Padding 1%
//        }
//    }
//    else {
//        expansionScale = 2.0f; // Fallback
//    }
//
//    // 4. Create Path & Brush
//    Gdiplus::GraphicsPath path;
//    REAL expandedR = rawR * expansionScale;
//    path.AddEllipse(rawCx - expandedR, rawCy - expandedR, 2 * expandedR, 2 * expandedR);
//    path.Transform(&finalMatrix);
//
//    Gdiplus::PathGradientBrush* pthGrBrush = new Gdiplus::PathGradientBrush(&path);
//    pthGrBrush->SetWrapMode(Gdiplus::WrapModeClamp);
//
//    Gdiplus::PointF focalPoint(rawFx, rawFy);
//    finalMatrix.TransformPoints(&focalPoint, 1);
//    pthGrBrush->SetCenterPoint(focalPoint);
//
//    // 5. Color Mapping & Padding (Double Padding: 0.0 và 1.0)
//    struct StopData { REAL pos; Gdiplus::Color col; };
//    std::vector<StopData> sortedStops;
//
//    for (const auto& stop : stops) {
//        SVGColor c = stop.getStopColor();
//        BYTE a = (BYTE)(stop.getStopOpacity() * c.getA());
//        Gdiplus::Color gdiColor(a, c.getR(), c.getG(), c.getB());
//
//        // Map: 1.0 (Center) -> 0.0 (Outer)
//        REAL gdiPos = 1.0f - (stop.getOffset() / expansionScale);
//        if (gdiPos > 1.0f) gdiPos = 1.0f;
//        if (gdiPos < 0.0f) gdiPos = 0.0f;
//
//        sortedStops.push_back({ gdiPos, gdiColor });
//    }
//
//    // Sort để xử lý padding chính xác
//    std::sort(sortedStops.begin(), sortedStops.end(),
//        [](const StopData& a, const StopData& b) { return a.pos < b.pos; });
//
//    if (!sortedStops.empty()) {
//        // PADDING BIÊN (0.0): Nếu brush mở rộng ra ngoài vùng gradient gốc, tô màu stop cuối
//        if (sortedStops.front().pos > 0.0001f) {
//            sortedStops.insert(sortedStops.begin(), { 0.0f, sortedStops.front().col });
//        }
//        // PADDING TÂM (1.0): Nếu stop đầu tiên (offset 0) không ở 1.0 do scale, 
//        // hoặc SVG không có offset 0, phải lấp đầy tâm.
//        if (sortedStops.back().pos < 0.9999f) {
//            sortedStops.push_back({ 1.0f, sortedStops.back().col });
//        }
//    }
//
//    // Convert sang mảng GDI+
//    std::vector<Gdiplus::Color> finalColors;
//    std::vector<REAL> finalPositions;
//    for (const auto& s : sortedStops) {
//        finalColors.push_back(s.col);
//        finalPositions.push_back(s.pos);
//    }
//
//    pthGrBrush->SetInterpolationColors(finalColors.data(), finalPositions.data(), (int)finalColors.size());
//
//    return pthGrBrush;
//}

#include "stdafx.h"
#include "SVGRadialGradient.h"
#include "SVGStop.h" // Đảm bảo include này
#include <algorithm>
#include <vector>
#include <cmath>
#include <sstream>
#include <string>

using namespace std;

// Helper: Parse số an toàn
static float parseFloat(const string& str) {
    if (str.empty()) return 0.0f;
    string s = str;
    for (char& c : s) if (c == ',') c = '.';
    stringstream ss(s);
    ss.imbue(std::locale::classic());
    float val = 0;
    ss >> val;
    return val;
}

// Helper: Parse Transform (Xoay tâm chuẩn)
static void applyTransformString(Gdiplus::Matrix* matrix, const string& transStr) {
    if (transStr.empty()) return;
    matrix->Reset();
    string s = transStr;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (c == ',' || c == '(' || c == ')') s[i] = ' ';
        else if (c == '-' && i > 0 && (isdigit(s[i - 1]) || s[i - 1] == '.')) { s.insert(i, " "); i++; }
    }
    stringstream ss(s);
    ss.imbue(std::locale::classic());
    string token;
    while (ss >> token) {
        if (token == "matrix") {
            float m[6]; for (int i = 0; i < 6; ++i) ss >> m[i];
            Gdiplus::Matrix temp(m[0], m[1], m[2], m[3], m[4], m[5]);
            matrix->Multiply(&temp, Gdiplus::MatrixOrderAppend);
        }
        else if (token == "translate") {
            float tx, ty = 0; ss >> tx; if (!(ss >> ty)) ty = 0;
            matrix->Translate(tx, ty, Gdiplus::MatrixOrderAppend);
        }
        else if (token == "scale") {
            float sx, sy; ss >> sx; if (!(ss >> sy)) sy = sx;
            matrix->Scale(sx, sy, Gdiplus::MatrixOrderAppend);
        }
        else if (token == "rotate") {
            float angle, cx = 0, cy = 0; ss >> angle;
            streampos oldPos = ss.tellg();
            if (ss >> cx >> cy) {
                matrix->Translate(-cx, -cy, Gdiplus::MatrixOrderAppend);
                matrix->Rotate(angle, Gdiplus::MatrixOrderAppend);
                matrix->Translate(cx, cy, Gdiplus::MatrixOrderAppend);
            }
            else { ss.clear(); ss.seekg(oldPos); matrix->Rotate(angle, Gdiplus::MatrixOrderAppend); }
        }
        else if (token == "skewX") {
            float angle; ss >> angle; matrix->Shear(tan(angle * 3.14159265f / 180.0f), 0, Gdiplus::MatrixOrderAppend);
        }
        else if (token == "skewY") {
            float angle; ss >> angle; matrix->Shear(0, tan(angle * 3.14159265f / 180.0f), Gdiplus::MatrixOrderAppend);
        }
    }
}

static float distSq(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2) {
    return (p1.X - p2.X) * (p1.X - p2.X) + (p1.Y - p2.Y) * (p1.Y - p2.Y);
}

SVGRadialGradient::SVGRadialGradient() {
    cx = 0.5f; cy = 0.5f; r = 0.5f; fx = 0.5f; fy = 0.5f;
}

void SVGRadialGradient::parseAttributes(xml_node<>* node) {
    // 1. Parse các thuộc tính cơ bản
    SVGGradient::parseAttributes(node);

    // 2. Tự parse Stops con (Sử dụng SVGStop đã sửa lỗi ở trên)
    // Code này đảm bảo stops được đọc chính xác từ node hiện tại
    if (node->first_node("stop")) {
        stops.clear();
        for (xml_node<>* child = node->first_node("stop"); child; child = child->next_sibling("stop")) {
            SVGStop stop;
            stop.parseAttributes(child); // Gọi hàm đã sửa ở file SVGStop.cpp
            stops.push_back(stop);
        }
    }

    // 3. Parse Attributes với parseFloat an toàn
    auto parseVal = [&](xml_attribute<>* attr, float defVal) {
        if (!attr) return defVal;
        string val = attr->value();
        // Trim spaces
        size_t first = val.find_first_not_of(" \t\r\n");
        if (string::npos != first) val = val.substr(first, val.find_last_not_of(" \t\r\n") - first + 1);

        float f = 0;
        if (!val.empty() && val.back() == '%') {
            val.pop_back();
            f = parseFloat(val) / 100.0f;
        }
        else {
            f = parseFloat(val);
        }
        return f;
        };

    cx = parseVal(node->first_attribute("cx"), 0.5f);
    cy = parseVal(node->first_attribute("cy"), 0.5f);
    r = parseVal(node->first_attribute("r"), 0.5f);
    fx = parseVal(node->first_attribute("fx"), cx);
    fy = parseVal(node->first_attribute("fy"), cy);

    if (xml_attribute<>* attr = node->first_attribute("gradientTransform")) {
        if (!this->gradientTransform) this->gradientTransform = new Gdiplus::Matrix();
        applyTransformString(this->gradientTransform, attr->value());
    }
}

Gdiplus::Brush* SVGRadialGradient::createBrush(const Gdiplus::RectF& boundingBox) {
    if (stops.empty()) return new Gdiplus::SolidBrush(Gdiplus::Color::Black);

    REAL rawCx = (REAL)cx; REAL rawCy = (REAL)cy; REAL rawR = (REAL)r;
    REAL rawFx = (REAL)fx; REAL rawFy = (REAL)fy;

    // Focal Point Clamping
    REAL dx = rawFx - rawCx; REAL dy = rawFy - rawCy;
    REAL dist = sqrt(dx * dx + dy * dy);
    if (rawR > 0 && dist >= rawR * 0.999f) {
        REAL scale = (rawR * 0.999f) / dist;
        rawFx = rawCx + dx * scale; rawFy = rawCy + dy * scale;
    }

    // Matrix
    Gdiplus::Matrix finalMatrix;
    if (gradientTransform) finalMatrix.Multiply(gradientTransform);
    if (gradientUnits != "userSpaceOnUse") {
        finalMatrix.Scale(boundingBox.Width, boundingBox.Height, Gdiplus::MatrixOrderAppend);
        finalMatrix.Translate(boundingBox.X, boundingBox.Y, Gdiplus::MatrixOrderAppend);
    }

    // Dynamic Expansion (2.0x Safety)
    REAL expansionScale = 1.0f;
    Gdiplus::Matrix inverseMatrix;
    inverseMatrix.Multiply(&finalMatrix);

    if (inverseMatrix.Invert() == Gdiplus::Status::Ok) {
        Gdiplus::PointF corners[4] = {
            Gdiplus::PointF(boundingBox.X, boundingBox.Y),
            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y),
            Gdiplus::PointF(boundingBox.X, boundingBox.Y + boundingBox.Height),
            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y + boundingBox.Height)
        };
        inverseMatrix.TransformPoints(corners, 4);
        float maxDistSq = 0.0f;
        Gdiplus::PointF centerP(rawCx, rawCy);
        for (int i = 0; i < 4; i++) {
            float d = distSq(centerP, corners[i]);
            if (d > maxDistSq) maxDistSq = d;
        }
        if (rawR > 0 && sqrt(maxDistSq) > rawR) expansionScale = (sqrt(maxDistSq) / rawR) * 2.0f;
    }
    else { expansionScale = 2.0f; }

    Gdiplus::GraphicsPath path;
    REAL expandedR = rawR * expansionScale;
    path.AddEllipse(rawCx - expandedR, rawCy - expandedR, 2 * expandedR, 2 * expandedR);
    path.Transform(&finalMatrix);

    Gdiplus::PathGradientBrush* pthGrBrush = new Gdiplus::PathGradientBrush(&path);
    pthGrBrush->SetWrapMode(Gdiplus::WrapModeClamp);
    pthGrBrush->SetGammaCorrection(TRUE);

    Gdiplus::PointF focalPoint(rawFx, rawFy);
    finalMatrix.TransformPoints(&focalPoint, 1);
    pthGrBrush->SetCenterPoint(focalPoint);

    // Color Mapping
    struct StopData { REAL pos; Gdiplus::Color col; };
    std::vector<StopData> sortedStops;
    for (const auto& stop : stops) {
        SVGColor c = stop.getStopColor();
        BYTE a = (BYTE)(stop.getStopOpacity() * c.getA());
        REAL gdiPos = 1.0f - (stop.getOffset() / expansionScale);
        if (gdiPos > 1.0f) gdiPos = 1.0f; if (gdiPos < 0.0f) gdiPos = 0.0f;
        sortedStops.push_back({ gdiPos, Gdiplus::Color(a, c.getR(), c.getG(), c.getB()) });
    }
    std::sort(sortedStops.begin(), sortedStops.end(), [](const StopData& a, const StopData& b) { return a.pos < b.pos; });

    // Padding
    if (!sortedStops.empty()) {
        if (sortedStops.front().pos > 0.001f) sortedStops.insert(sortedStops.begin(), { 0.0f, sortedStops.front().col });
        if (sortedStops.back().pos < 0.999f) sortedStops.push_back({ 1.0f, sortedStops.back().col });
    }

    std::vector<Gdiplus::Color> finalColors;
    std::vector<REAL> finalPositions;
    for (const auto& s : sortedStops) { finalColors.push_back(s.col); finalPositions.push_back(s.pos); }
    pthGrBrush->SetInterpolationColors(finalColors.data(), finalPositions.data(), (int)finalColors.size());

    // Fix viền
    if (!finalColors.empty()) {
        int count = pthGrBrush->GetPointCount();
        if (count > 0) {
            Gdiplus::Color* surroundColors = new Gdiplus::Color[count];
            Gdiplus::Color outerColor = finalColors[0];
            for (int i = 0; i < count; i++) surroundColors[i] = outerColor;
            pthGrBrush->SetSurroundColors(surroundColors, &count);
            delete[] surroundColors;
        }
    }
    return pthGrBrush;
}