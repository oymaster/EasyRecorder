#include "AreaSelectorButtons.h"

AreaSelectorButtons::AreaSelectorButtons()
{
}

AreaSelectorButtons::~AreaSelectorButtons()
{
}

// 只绘制圆形边框，不填充颜色
QPixmap AreaSelectorButtons::getButton()
{
    QPixmap pixmap(diameter + penWidth, diameter + penWidth);
    pixmap.fill(Qt::transparent);  // 透明背景

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing, true);

    QBrush brush;
    brush.setStyle(Qt::NoBrush);  // 取消填充，让圆变透明
    painter.setBrush(brush);

    QPen pen;
    pen.setColor(Qt::black);   // 边框颜色
    pen.setWidth(penWidth);    // 边框宽度
    painter.setPen(pen);

    // 仅绘制圆形边框
    painter.drawEllipse(penWidthHalf, penWidthHalf, diameter, diameter);

    return pixmap;
}

// 绘制箭头
QPixmap AreaSelectorButtons::getArrow(degreeArrow degree)
{
    QPixmap pixmap(diameter + penWidth, diameter + penWidth);
    pixmap.fill(Qt::transparent);  // 透明背景

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.translate((diameter + penWidth) / 2, (diameter + penWidth) / 2);
    painter.rotate(degree);

    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(colorSelected);
    pen.setWidthF(penWidth);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);

    QBrush brush;
    brush.setColor(colorSelected);
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);

    QPainterPath painterPath;
    painterPath.moveTo(0, 0);
    painterPath.lineTo(0, -radius + penWidth);
    painterPath.lineTo(-3, -radius + penWidth + 7);
    painterPath.lineTo(3, -radius + penWidth + 7);
    painterPath.lineTo(0, -radius + penWidth);

    painter.drawPath(painterPath);

    return pixmap;
}

// 组合箭头和边框圆
QPixmap AreaSelectorButtons::getPixmapHandle(degreeArrow degree)
{
    QPixmap pixmap(diameter + penWidth, diameter + penWidth);
    pixmap.fill(Qt::transparent);  // 透明背景

    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing, true);

    // 仅绘制圆形边框，不填充
    painter.drawPixmap(0, 0, getButton());

    // 叠加箭头
    painter.drawPixmap(0, 0, getArrow(degree));

    return pixmap;
}

int AreaSelectorButtons::getWithHalf()
{
    return (diameter + penWidth) / 2;
}
