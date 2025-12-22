#include "stdafx.h"
#include "SVGRadialGradient.h"

SVGRadialGradient::SVGRadialGradient() {
    cx = 0.5f; cy = 0.5f; r = 0.5f;
    fx = 0.5f; fy = 0.5f;
}

void SVGRadialGradient::parseAttributes(xml_node<>* node) {
    SVGGradient::parseAttributes(node); // Parse stops và xlink:href

    auto parseVal = [&](xml_attribute<>* attr, float defVal) {
        if (!attr) return defVal;
        string val = attr->value();
        if (val.back() == '%') {
            val.pop_back();
            return (float)atof(val.c_str()) / 100.0f;
        }
        return (float)atof(val.c_str());
        };

    cx = parseVal(node->first_attribute("cx"), 0.5f);
    cy = parseVal(node->first_attribute("cy"), 0.5f);
    r = parseVal(node->first_attribute("r"), 0.5f);
    fx = parseVal(node->first_attribute("fx"), cx);
    fy = parseVal(node->first_attribute("fy"), cy);
}

Gdiplus::Brush* SVGRadialGradient::createBrush(const Gdiplus::RectF& boundingBox) {
    // FALLBACK: Tính màu trung bình cộng của các stops
    if (stops.empty()) {
        return new Gdiplus::SolidBrush(Gdiplus::Color::Black);
    }

    int rSum = 0, gSum = 0, bSum = 0, aSum = 0;
    for (const auto& stop : stops) {
        SVGColor c = stop.getStopColor();
        rSum += c.getR();
        gSum += c.getG();
        bSum += c.getB();
        aSum += (int)(stop.getStopOpacity() * c.getA());
    }

    int n = (int)stops.size();
    Gdiplus::Color averageColor(
        (BYTE)(aSum / n),
        (BYTE)(rSum / n),
        (BYTE)(gSum / n),
        (BYTE)(bSum / n)
    );

    return new Gdiplus::SolidBrush(averageColor);
}