#ifndef SPARKLINEDELEGATE_H
#define SPARKLINEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QVector>

#pragma execution_character_set("utf-8")

// 可配置的历史数据点数量
const int MAX_HISTORY_POINTS = 30;

// 自定义委托：在单元格中绘制迷你曲线图（Sparkline）
class SparklineDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SparklineDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        // 获取存储在单元格中的历史数据
        QVariant data = index.data(Qt::UserRole);
        if (!data.isValid() || !data.canConvert<QVector<double>>())
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QVector<double> values = data.value<QVector<double>>();
        if (values.isEmpty())
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        painter->save();

        // 绘制背景
        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }
        else
        {
            painter->fillRect(option.rect, option.palette.base());
        }

        // 计算绘图区域（留出边距）
        QRect plotRect = option.rect.adjusted(4, 4, -4, -4);
        if (plotRect.width() < 10 || plotRect.height() < 10)
        {
            painter->restore();
            return;
        }

        // 找到数据的最大最小值
        double minVal = values.first();
        double maxVal = values.first();
        for (double v : values)
        {
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }

        // 添加一些边距，避免曲线贴边
        double range = maxVal - minVal;
        if (range < 0.01) range = 1.0;  // 防止除以零
        minVal -= range * 0.1;
        maxVal += range * 0.1;
        range = maxVal - minVal;

        // 绘制参考线（中间虚线）
        painter->setPen(QPen(QColor(200, 200, 200), 1, Qt::DashLine));
        int midY = plotRect.top() + plotRect.height() / 2;
        painter->drawLine(plotRect.left(), midY, plotRect.right(), midY);

        // 绘制曲线
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPen linePen(QColor(30, 144, 255), 2);  // 道奇蓝色
        painter->setPen(linePen);

        QVector<QPointF> points;
        int n = values.size();
        for (int i = 0; i < n; ++i)
        {
            double x = plotRect.left() + (double)i / qMax(1, n - 1) * plotRect.width();
            double y = plotRect.bottom() - (values[i] - minVal) / range * plotRect.height();
            points.append(QPointF(x, y));
        }

        // 绘制折线
        for (int i = 0; i < points.size() - 1; ++i)
        {
            painter->drawLine(points[i], points[i + 1]);
        }

        // 绘制数据点
        painter->setBrush(QColor(30, 144, 255));
        painter->setPen(Qt::NoPen);
        for (const QPointF &pt : points)
        {
            painter->drawEllipse(pt, 3, 3);
        }

        // 高亮最后一个点（当前值）
        if (!points.isEmpty())
        {
            painter->setBrush(QColor(255, 69, 0));  // 橙红色
            painter->drawEllipse(points.last(), 4, 4);
        }

        // 在曲线图中显示最大最小值
        if (!values.isEmpty())
        {
            // 重新计算原始的最大最小值（不含边距）
            double origMin = values.first();
            double origMax = values.first();
            for (double v : values)
            {
                if (v < origMin) origMin = v;
                if (v > origMax) origMax = v;
            }
            
            painter->setPen(Qt::black);
            QFont font = painter->font();
            font.setPointSize(8);
            painter->setFont(font);
            
            // 左上角显示最大值
            QString maxText = QString("max:%1").arg(origMax, 0, 'f', 1);
            painter->drawText(plotRect.adjusted(2, 0, 0, 0), Qt::AlignTop | Qt::AlignLeft, maxText);
            
            // 左下角显示最小值
            QString minText = QString("min:%1").arg(origMin, 0, 'f', 1);
            painter->drawText(plotRect.adjusted(2, 0, 0, -2), Qt::AlignBottom | Qt::AlignLeft, minText);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(150, 50);  // 建议的单元格大小
    }
};

#endif // SPARKLINEDELEGATE_H
