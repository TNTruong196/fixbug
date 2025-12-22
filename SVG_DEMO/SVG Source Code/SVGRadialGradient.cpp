//#include "stdafx.h"
//#include "SVGRadialGradient.h"
//#include <algorithm>
//#include <vector>
//#include <cmath>
//
//SVGRadialGradient::SVGRadialGradient() {
//    cx = 0.5f; cy = 0.5f; r = 0.5f;
//    fx = 0.5f; fy = 0.5f;
//}
//
//void SVGRadialGradient::parseAttributes(xml_node<>* node) {
//    SVGGradient::parseAttributes(node); // Parse stops, transforms, id, xlink:href
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
//    // Parse các thuộc tính, mặc định fx, fy theo cx, cy nếu thiếu
//    cx = parseVal(node->first_attribute("cx"), 0.5f);
//    cy = parseVal(node->first_attribute("cy"), 0.5f);
//    r = parseVal(node->first_attribute("r"), 0.5f);
//    fx = parseVal(node->first_attribute("fx"), cx);
//    fy = parseVal(node->first_attribute("fy"), cy);
//}
//
//Gdiplus::Brush* SVGRadialGradient::createBrush(const Gdiplus::RectF& boundingBox) {
//    if (stops.empty()) {
//        return new Gdiplus::SolidBrush(Gdiplus::Color::Black);
//    }
//
//    // 1. Chuẩn bị thông số hình học gốc
//    REAL rawCx = (REAL)cx;
//    REAL rawCy = (REAL)cy;
//    REAL rawR = (REAL)r;
//    REAL rawFx = (REAL)fx;
//    REAL rawFy = (REAL)fy;
//
//    // 2. Tính toán Matrix biến đổi tổng hợp
//    Gdiplus::Matrix transformMatrix;
//
//    // Áp dụng gradientTransform của SVG trước (nếu có)
//    if (gradientTransform) {
//        transformMatrix.Multiply(gradientTransform);
//    }
//
//    // Xử lý đơn vị toạ độ
//    if (gradientUnits == "objectBoundingBox") {
//        // Scale từ 0..1 lên kích thước boundingBox
//        Gdiplus::Matrix mapMatrix;
//        mapMatrix.Scale(boundingBox.Width, boundingBox.Height, Gdiplus::MatrixOrderAppend);
//        mapMatrix.Translate(boundingBox.X, boundingBox.Y, Gdiplus::MatrixOrderAppend);
//        transformMatrix.Multiply(&mapMatrix, Gdiplus::MatrixOrderAppend);
//    }
//    // Nếu là userSpaceOnUse, rawCx/Cy đã là pixel, transformMatrix chỉ chứa gradientTransform.
//
//    // 3. KỸ THUẬT MỞ RỘNG BÁN KÍNH (INFINITE GRADIENT SIMULATION)
//    // Để tránh việc PathGradientBrush bị cắt cụt (clipping) ở rìa hình tròn,
//    // ta nhân bán kính lên 50 lần để brush phủ kín mọi hình dạng (Instagram box).
//    // Màu sắc sẽ được "nén" lại vào tâm để giữ đúng tỷ lệ hiển thị.
//    REAL expansionScale = 50.0f;
//
//    Gdiplus::GraphicsPath path;
//    REAL expandedR = rawR * expansionScale;
//
//    // Tạo hình Ellipse cực lớn
//    path.AddEllipse(rawCx - expandedR, rawCy - expandedR, 2 * expandedR, 2 * expandedR);
//
//    // Áp dụng biến đổi (xoay/nghiêng/dịch) lên hình Ellipse lớn này
//    path.Transform(&transformMatrix);
//
//    // Tạo Brush từ Path lớn
//    Gdiplus::PathGradientBrush* pthGrBrush = new Gdiplus::PathGradientBrush(&path);
//
//    // Tính điểm tiêu cự (Focal Point)
//    // Lưu ý: Focal Point cũng phải được transform nhưng KHÔNG nhân với expansionScale
//    // (vì nó nằm tương đối so với tâm thực sự của gradient)
//    Gdiplus::PointF centerPoint(rawFx, rawFy);
//    transformMatrix.TransformPoints(&centerPoint, 1);
//
//    // GDI+ yêu cầu CenterPoint nằm trong Path. Vì Path đã scale to 50 lần, điều này luôn đúng.
//    pthGrBrush->SetCenterPoint(centerPoint);
//
//    // 4. Xử lý và Map lại màu sắc (Stops Remapping)
//    struct StopData { REAL pos; Gdiplus::Color col; };
//    std::vector<StopData> sortedStops;
//
//    for (const auto& stop : stops) {
//        SVGColor c = stop.getStopColor();
//        BYTE a = (BYTE)(stop.getStopOpacity() * c.getA());
//        Gdiplus::Color gdiColor(a, c.getR(), c.getG(), c.getB());
//
//        // Mapping Logic:
//        // SVG Offset: 0.0 (Tâm) -> 1.0 (Rìa bán kính r gốc)
//        // GDI Position: 1.0 (Tâm) -> 0.0 (Rìa bán kính 50*r)
//        //
//        // Công thức chuyển đổi:
//        // Khoảng cách vật lý d = offset * r
//        // Tỷ lệ trên brush mới t = d / (r * 50) = offset / 50
//        // GDI Position = 1.0 - t = 1.0 - (offset / 50)
//
//        REAL gdiPos = 1.0f - (stop.getOffset() / expansionScale);
//
//        // Clamp an toàn
//        if (gdiPos > 1.0f) gdiPos = 1.0f;
//        if (gdiPos < 0.0f) gdiPos = 0.0f;
//
//        sortedStops.push_back({ gdiPos, gdiColor });
//    }
//
//    // 5. Thêm điểm dừng "Pad" (Mở rộng màu nền)
//    // Để phần mở rộng (từ r đến 50*r) có màu của điểm cuối cùng (giả lập spreadMethod="pad")
//    // ta tìm stop nằm xa tâm nhất (offset lớn nhất => gdiPos nhỏ nhất)
//    if (!sortedStops.empty()) {
//        auto minEl = std::min_element(sortedStops.begin(), sortedStops.end(),
//            [](const StopData& a, const StopData& b) { return a.pos < b.pos; });
//
//        // Thêm một stop tại vị trí 0.0 (biên ngoài cùng của brush khổng lồ)
//        // với màu giống hệt màu ngoài cùng của gradient gốc.
//        sortedStops.push_back({ 0.0f, minEl->col });
//    }
//
//    // GDI+ yêu cầu sắp xếp Position tăng dần (0 -> 1)
//    std::sort(sortedStops.begin(), sortedStops.end(),
//        [](const StopData& a, const StopData& b) { return a.pos < b.pos; });
//
//    // Copy dữ liệu sang mảng để gán cho Brush
//    std::vector<Gdiplus::Color> finalColors;
//    std::vector<REAL> finalPositions;
//    for (const auto& s : sortedStops) {
//        finalColors.push_back(s.col);
//        finalPositions.push_back(s.pos);
//    }
//
//    pthGrBrush->SetInterpolationColors(finalColors.data(), finalPositions.data(), (int)finalColors.size());
//
//    // Không cần delete path vì Brush đã copy dữ liệu
//    return pthGrBrush;
//}

#include "stdafx.h"
#include "SVGRadialGradient.h"
#include <algorithm>
#include <vector>
#include <cmath>

SVGRadialGradient::SVGRadialGradient() {
    cx = 0.5f; cy = 0.5f; r = 0.5f;
    fx = 0.5f; fy = 0.5f;
}

void SVGRadialGradient::parseAttributes(xml_node<>* node) {
    SVGGradient::parseAttributes(node);

    auto parseVal = [&](xml_attribute<>* attr, float defVal) {
        if (!attr) return defVal;
        string val = attr->value();
        if (val.back() == '%') {
            val.pop_back();
            return (float)atof(val.c_str()) / 100.0f;
        }
        return (float)atof(val.c_str());
        };

    // Parse attributes
    cx = parseVal(node->first_attribute("cx"), 0.5f);
    cy = parseVal(node->first_attribute("cy"), 0.5f);
    r = parseVal(node->first_attribute("r"), 0.5f);

    // Nếu fx, fy không có, mặc định trùng với cx, cy
    fx = parseVal(node->first_attribute("fx"), cx);
    fy = parseVal(node->first_attribute("fy"), cy);
}

// Hàm tính khoảng cách giữa 2 điểm
static float distSq(const Gdiplus::PointF& p1, const Gdiplus::PointF& p2) {
    return (p1.X - p2.X) * (p1.X - p2.X) + (p1.Y - p2.Y) * (p1.Y - p2.Y);
}

Gdiplus::Brush* SVGRadialGradient::createBrush(const Gdiplus::RectF& boundingBox) {
    if (stops.empty()) {
        return new Gdiplus::SolidBrush(Gdiplus::Color::Black);
    }

    // 1. Xây dựng Ma trận biến đổi tổng hợp (Final Transformation Matrix)
    // Ma trận này chuyển đổi từ toạ độ Gradient (0..1 hoặc userSpace) sang toạ độ Pixel màn hình.
    Gdiplus::Matrix finalMatrix;

    // a. Áp dụng gradientTransform của SVG (nếu có)
    if (gradientTransform) {
        finalMatrix.Multiply(gradientTransform);
    }

    // b. Xử lý mapping vào BoundingBox (nếu là objectBoundingBox)
    if (gradientUnits == "objectBoundingBox") {
        // Lưu ý thứ tự nhân ma trận: Transform SVG -> Scale theo Box -> Dịch chuyển theo Box
        // Sử dụng MatrixOrderAppend để nhân vào phía sau chuỗi biến đổi hiện tại
        finalMatrix.Scale(boundingBox.Width, boundingBox.Height, Gdiplus::MatrixOrderAppend);
        finalMatrix.Translate(boundingBox.X, boundingBox.Y, Gdiplus::MatrixOrderAppend);
    }

    // 2. TÍNH TOÁN HỆ SỐ MỞ RỘNG (DYNAMIC EXPANSION)
    // Mục tiêu: Tìm xem hình chữ nhật (BoundingBox) khi chiếu ngược vào không gian Gradient
    // thì nó nằm xa tâm (cx, cy) bao nhiêu.

    REAL expansionScale = 1.0f;
    REAL rawCx = (REAL)cx;
    REAL rawCy = (REAL)cy;
    REAL rawR = (REAL)r;

    // Tạo ma trận nghịch đảo để chiếu ngược điểm từ Pixel về Gradient Space
    Gdiplus::Matrix inverseMatrix;
    inverseMatrix.Multiply(&finalMatrix);

    if (inverseMatrix.Invert() == Gdiplus::Status::Ok) {
        // Lấy 4 góc của BoundingBox
        Gdiplus::PointF corners[4] = {
            Gdiplus::PointF(boundingBox.X, boundingBox.Y),
            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y),
            Gdiplus::PointF(boundingBox.X, boundingBox.Y + boundingBox.Height),
            Gdiplus::PointF(boundingBox.X + boundingBox.Width, boundingBox.Y + boundingBox.Height)
        };

        // Chiếu 4 góc này về không gian Gradient gốc
        inverseMatrix.TransformPoints(corners, 4);

        // Tìm khoảng cách xa nhất từ tâm (cx, cy) đến 4 góc này
        float maxDistSq = 0.0f;
        Gdiplus::PointF centerP(rawCx, rawCy);
        for (int i = 0; i < 4; i++) {
            float d = distSq(centerP, corners[i]);
            if (d > maxDistSq) maxDistSq = d;
        }

        float maxDist = sqrt(maxDistSq);

        // Nếu điểm xa nhất nằm ngoài bán kính r, ta phải mở rộng brush
        // Thêm một chút padding (1.01) để tránh lỗi làm tròn số ở viền
        if (rawR > 0 && maxDist > rawR) {
            expansionScale = (maxDist / rawR) * 1.01f;
        }
    }
    else {
        // Trường hợp hiếm: Ma trận không thể nghịch đảo (ví dụ scale = 0), dùng fallback an toàn
        expansionScale = 2.0f;
    }

    // 3. Tạo GraphicsPath cho Brush
    // Ta tạo một hình Ellipse tại toạ độ gốc nhưng với bán kính ĐÃ ĐƯỢC MỞ RỘNG
    Gdiplus::GraphicsPath path;
    REAL expandedR = rawR * expansionScale;

    // Tạo hình tròn cơ sở
    path.AddEllipse(rawCx - expandedR, rawCy - expandedR, 2 * expandedR, 2 * expandedR);

    // Biến đổi hình tròn này ra toạ độ màn hình bằng ma trận đã tính
    path.Transform(&finalMatrix);

    // 4. Tạo PathGradientBrush
    Gdiplus::PathGradientBrush* pthGrBrush = new Gdiplus::PathGradientBrush(&path);

    // 5. Thiết lập điểm tiêu cự (Focal Point)
    // Focal Point (fx, fy) cũng phải được transform ra toạ độ màn hình
    Gdiplus::PointF focalPoint((REAL)fx, (REAL)fy);
    finalMatrix.TransformPoints(&focalPoint, 1);

    // SetCenterPoint trong GDI+ tương đương với Focal Point trong SVG (nơi bắt đầu màu offset=0)
    pthGrBrush->SetCenterPoint(focalPoint);

    // 6. Map lại màu (Color Stops Remapping)
    struct StopData { REAL pos; Gdiplus::Color col; };
    std::vector<StopData> sortedStops;

    for (const auto& stop : stops) {
        SVGColor c = stop.getStopColor();
        BYTE a = (BYTE)(stop.getStopOpacity() * c.getA());
        Gdiplus::Color gdiColor(a, c.getR(), c.getG(), c.getB());

        // Công thức map màu khi đã mở rộng bán kính:
        // Vị trí thực tế = offset gốc / tỷ lệ mở rộng
        // Đảo ngược cho GDI+ (1.0 là tâm, 0.0 là biên)
        REAL gdiPos = 1.0f - (stop.getOffset() / expansionScale);

        if (gdiPos > 1.0f) gdiPos = 1.0f;
        if (gdiPos < 0.0f) gdiPos = 0.0f;

        sortedStops.push_back({ gdiPos, gdiColor });
    }

    // Xử lý PAD (Màu nền):
    // Vì ta đã mở rộng brush ra quá bán kính gốc r, phần dư thừa cần được tô bằng màu cuối cùng.
    // Màu cuối cùng (offset=1.0) sau khi map sẽ nằm ở vị trí (1.0 - 1.0/scale).
    // Ta cần thêm một stop ở biên ngoài cùng (pos = 0.0) có màu giống màu đó.
    if (!sortedStops.empty()) {
        // Tìm stop có offset lớn nhất (tức là gdiPos nhỏ nhất)
        auto minEl = std::min_element(sortedStops.begin(), sortedStops.end(),
            [](const StopData& a, const StopData& b) { return a.pos < b.pos; });

        // Nếu stop xa nhất chưa chạm tới biên (0.0), lấp đầy khoảng trống bằng màu đó
        if (minEl->pos > 0.0001f) {
            sortedStops.push_back({ 0.0f, minEl->col });
        }
    }

    // Sắp xếp lại stops theo thứ tự tăng dần (bắt buộc của GDI+)
    std::sort(sortedStops.begin(), sortedStops.end(),
        [](const StopData& a, const StopData& b) { return a.pos < b.pos; });

    // Copy sang mảng GDI+
    std::vector<Gdiplus::Color> finalColors;
    std::vector<REAL> finalPositions;
    for (const auto& s : sortedStops) {
        finalColors.push_back(s.col);
        finalPositions.push_back(s.pos);
    }

    pthGrBrush->SetInterpolationColors(finalColors.data(), finalPositions.data(), (int)finalColors.size());

    return pthGrBrush;
}