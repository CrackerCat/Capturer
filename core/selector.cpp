#include "selector.h"
#include <QMouseEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QApplication>
#include <QShortcut>
#include <QDesktopWidget>
#include "detectwidgets.h"

Selector::Selector(QWidget * parent)
    : QWidget(parent), Resizer()
{
    info_ = new Info(this);         // ???? 放在构造函数的末尾会造成全屏失败 ????

    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);

//    setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
    setMaxSize();

    connect(this, &Selector::moved, [this](){ update(); });
    connect(this, &Selector::resized, [this](){ update(); });
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, [this](){ setMaxSize(); });

    registerShortcuts();
}

void Selector::setMaxSize()
{
    auto screens = QGuiApplication::screens();

    int width = 0, height = 0;
    foreach(const auto& screen, screens) {
        auto geometry = screen->geometry();
        width += geometry.width();
        if(height < geometry.height()) height = geometry.height();
    }
    setFixedSize(width, height);
}

void Selector::start()
{
    if(status_ == INITIAL) {
        status_ = NORMAL;
        show();

        if(use_detect_) {
            auto rect = DetectWidgets::window();
            x1_ = rect.left();
            x2_ = rect.right();
            y1_ = rect.top();
            y2_ = rect.bottom();
            info_->show();
        }
    }
}

void Selector::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        cursor_pos_ = position(event->pos());

        switch (status_) {
        case NORMAL:
        {
            auto pos = event->pos();
            x2_ = x1_ = pos.x();
            y2_ = y1_ = pos.y();
            info_->show();

            status_ = SELECTING;
            break;
        }

        case SELECTING: break;

        case CAPTURED:
            if(cursor_pos_ == INSIDE) {
                mbegin_ = event->pos();
                mend_ = event->pos();

                status_ = MOVING;
            }
            else {
                rbegin_ = event->pos();
                rend_ = event->pos();

                status_ = RESIZING;
            }
            break;

        case MOVING:
        case RESIZING:
        case LOCKED:
        default: break;
        }
    }

    QWidget::mousePressEvent(event);
}

void Selector::mouseMoveEvent(QMouseEvent* event)
{
    auto mouse_pos = event->pos();

    switch (status_) {
    case NORMAL: setCursor(Qt::CrossCursor); break;
    case SELECTING:
        x2_ = mouse_pos.x();
        y2_ = mouse_pos.y();

        status_ = SELECTING;
        break;

    case CAPTURED:
        cursor_pos_ = position(mouse_pos);
        switch (cursor_pos_) {
        case INSIDE:  setCursor(Qt::SizeAllCursor); break;
        case OUTSIDE: setCursor(Qt::ForbiddenCursor); break;

        case Y1_ANCHOR:
        case Y2_ANCHOR:
        case Y1_BORDER:
        case Y2_BORDER: setCursor(Qt::SizeVerCursor); break;

        case X1_ANCHOR:
        case X2_ANCHOR:
        case X1_BORDER:
        case X2_BORDER: setCursor(Qt::SizeHorCursor); break;

        case X1Y1_ANCHOR:
            ((x1_ < x2_ && y1_ < y2_) || (x1_ > x2_ && y1_ > y2_))
                    ? setCursor(Qt::SizeFDiagCursor)
                    : setCursor(Qt::SizeBDiagCursor);
            break;

        case X1Y2_ANCHOR:
            ((x1_ < x2_ && y2_ < y1_) || (x1_ > x2_ && y2_ > y1_))
                ? setCursor(Qt::SizeFDiagCursor)
                : setCursor(Qt::SizeBDiagCursor);
            break;

        case X2Y1_ANCHOR:
            ((x1_ < x2_ && y2_ < y1_) || (x1_ > x2_ && y2_ > y1_))
                ? setCursor(Qt::SizeFDiagCursor)
                : setCursor(Qt::SizeBDiagCursor);
            break;

        case X2Y2_ANCHOR:
            ((x2_ < x1_ && y2_ < y1_) || (x2_ > x1_ && y2_ > y1_))
                    ? setCursor(Qt::SizeFDiagCursor)
                    : setCursor(Qt::SizeBDiagCursor);
            break;

        default: break;
        }
        break;

    case MOVING:
        mend_ = mouse_pos;
        updateSelected();
        mbegin_ = mouse_pos;

        status_ = MOVING;
        break;

    case RESIZING:
        rend_ = mouse_pos;
        updateSelected();
        rbegin_ = mouse_pos;

        status_ = RESIZING;
        break;

    case LOCKED:
    default: break;
    }

    this->update();     // TODO: 减少刷新次数，提升性能
    QWidget::mouseMoveEvent(event);
}

void Selector::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        switch (status_) {
        case NORMAL:break;
        case SELECTING:
            x2_ = event->pos().x();
            y2_ = event->pos().y();

            // detected window
            if(x1_ == x2_ && y1_ == y2_ && use_detect_) {
                auto window = DetectWidgets::window();
                x1_ = window.left();
                x2_ = window.right();
                y1_ = window.top();
                y2_ = window.bottom();
            }
            status_ = CAPTURED;
            break;
        case MOVING: mend_ = event->pos(); status_ = CAPTURED; break;
        case RESIZING: rend_ = event->pos(); status_ = CAPTURED; break;
        case CAPTURED:
        case LOCKED:
        default: break;
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void Selector::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
}

void Selector::exit()
{
    status_ = INITIAL;
    x1_ = x2_ = y1_ = y2_ = 0;
    info_->hide();

    repaint();

    QWidget::hide();
}

void Selector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    painter_.begin(this);

    auto srect = selected();

    painter_.fillRect(rect(), QColor(0, 0, 0, 1)); // Make Windows(OS) happy.

    painter_.fillRect(QRect{ 0, 0, width(), srect.y() }, mask_color_);
    painter_.fillRect(QRect{ 0, srect.y(), srect.x(), srect.height() }, mask_color_);
    painter_.fillRect(QRect{ srect.x() + srect.width(), srect.y(), width() - srect.x() - srect.width(), srect.height()}, mask_color_);
    painter_.fillRect(QRect{ 0, srect.y() + srect.height(), width(), height() - srect.y() - srect.height()}, mask_color_);

    if(status_ == NORMAL && use_detect_) {
        auto rect = DetectWidgets::window();
        x1_ = rect.left();
        x2_ = rect.right();
        y1_ = rect.top();
        y2_ = rect.bottom();
    }

    if(use_detect_ || status_ > NORMAL) {
        // info
        info_->size(selected().size());
        auto info_y = topLeft().y() - info_->geometry().height();
        info_->move(topLeft().x() + 1, (info_y < 0 ? topLeft().y() + 1 : info_y));

        // draw border
        painter_.setPen(QPen(border_color_, border_width_, border_style_));
        painter_.drawRect(srect);

        // draw anchor
        painter_.fillRect(X1Y1Anchor(), border_color_);
        painter_.fillRect(X1Y2Anchor(), border_color_);
        painter_.fillRect(X2Y1Anchor(), border_color_);
        painter_.fillRect(X2Y2Anchor(), border_color_);

        painter_.fillRect(Y1Anchor(), border_color_);
        painter_.fillRect(X2Anchor(), border_color_);
        painter_.fillRect(Y2Anchor(), border_color_);
        painter_.fillRect(X1Anchor(), border_color_);
    }
    painter_.end();
}

void Selector::updateSelected()
{
    if(status_ == MOVING) {
        QPoint diff(mend_.x() - mbegin_.x(), mend_.y() - mbegin_.y());

        auto diff_min_x = std::max(l(), 0);
        auto diff_max_x = std::max(width() - r(), 0);
        auto diff_min_y = std::max(t(), 0);
        auto diff_max_y = std::max(height() - b(), 0);

        diff.rx() = (diff.x() < 0) ? std::max(diff.x(), -diff_min_x) : std::min(diff.x(), diff_max_x);
        diff.ry() = (diff.y() < 0) ? std::max(diff.y(), -diff_min_y) : std::min(diff.y(), diff_max_y);

        x1_ += diff.x();
        x2_ += diff.x();
        y1_ += diff.y();
        y2_ += diff.y();
    }
    else if(status_ == RESIZING) {
        auto diff_x = rend_.x() - rbegin_.x();
        auto diff_y = rend_.y() - rbegin_.y();

        switch (cursor_pos_) {
        case Y1_BORDER: case Y1_ANCHOR: y1_ += diff_y; break;
        case Y2_BORDER: case Y2_ANCHOR: y2_ += diff_y; break;
        case X1_BORDER: case X1_ANCHOR: x1_ += diff_x; break;
        case X2_BORDER: case X2_ANCHOR: x2_ += diff_x; break;

        case X1Y1_ANCHOR: x1_ += diff_x; y1_ += diff_y; break;
        case X1Y2_ANCHOR: x1_ += diff_x; y2_ += diff_y; break;
        case X2Y1_ANCHOR: x2_ += diff_x; y1_ += diff_y; break;
        case X2Y2_ANCHOR: x2_ += diff_x; y2_ += diff_y; break;

        default:break;
        }
    }
}

void Selector::registerShortcuts()
{
    // move
    auto move_up = new QShortcut(QKeySequence("W"), this);
    connect(move_up, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED && y1_ > 0 && y2_ > 0) {
            y1_ -= 1;
            y2_ -= 1;
            emit moved();
        }
    });

    auto move_down = new QShortcut(QKeySequence("S"), this);
    connect(move_down, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED && y1_ < height() && y2_ < height()) {
            y1_ += 1;
            y2_ += 1;
            emit moved();
        }
    });

    auto move_left = new QShortcut(QKeySequence("A"), this);
    connect(move_left, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED && x1_ > 0 && x2_ > 0) {
            x1_ -= 1;
            x2_ -= 1;
            emit moved();
        }
    });

    auto move_right = new QShortcut(QKeySequence("D"), this);
    connect(move_right, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED && x1_ < width() && x2_ < width()) {
            x1_ += 1;
            x2_ += 1;
            emit moved();
        }
    });

    // resize
    // increase
    auto increase_top = new QShortcut(Qt::CTRL + Qt::Key_Up, this);
    connect(increase_top, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            y1_ < y2_ ? y1_ = std::max(y1_ - 1, 0) : y2_ = std::max(y2_ - 1, 0);
            emit resized();
        }
    });

    auto increase_bottom = new QShortcut(Qt::CTRL + Qt::Key_Down, this);
    connect(increase_bottom, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            y1_ > y2_ ? y1_ = std::min(y1_ + 1, height()) : y2_ = std::min(y2_ + 1, height());
            emit resized();
        }
    });

    auto increase_left = new QShortcut(Qt::CTRL + Qt::Key_Left, this);
    connect(increase_left, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            x1_ < x2_ ? x1_ = std::max(x1_ - 1, 0) : x2_ = std::max(x2_ - 1, 0);
            emit resized();
        }
    });

    auto increase_right = new QShortcut(Qt::CTRL + Qt::Key_Right, this);
    connect(increase_right, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            x1_ > x2_ ? x1_ = std::min(x1_ + 1, width()) : x2_ = std::min(x2_ + 1, width());
            emit resized();
        }
    });

    // decrease
    auto decrease_top = new QShortcut(Qt::SHIFT + Qt::Key_Up, this);
    connect(decrease_top, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            y1_ < y2_ ? y1_ = std::min(y1_ + 1, y2_) : y2_ = std::min(y2_ + 1, y1_);
            emit resized();
        }
    });

    auto decrease_bottom = new QShortcut(Qt::SHIFT + Qt::Key_Down, this);
    connect(decrease_bottom, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            y1_ > y2_ ? y1_ = std::max(y1_ - 1, y2_) : y2_ = std::max(y2_ - 1, y1_);
            emit resized();
        }
    });

    auto decrease_left = new QShortcut(Qt::SHIFT + Qt::Key_Left, this);
    connect(decrease_left, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            x1_ < x2_ ? x1_ = std::min(x1_ + 1, x2_) : x2_ = std::min(x2_ + 1, x1_);
            emit resized();
        }
    });

    auto decrease_right = new QShortcut(Qt::SHIFT + Qt::Key_Right, this);
    connect(decrease_right, &QShortcut::activated, [&]() {
        if(status_ == CAPTURED) {
            x1_ > x2_ ? x1_ = std::max(x1_ - 1, x2_) : x2_ = std::max(x2_ - 1, x1_);
            emit resized();
        }
    });

    auto select_all = new QShortcut(Qt::CTRL + Qt::Key_A, this);
    connect(select_all, &QShortcut::activated, [&]() {
        x1_ = 0; x2_ = rect().width();
        y1_ = 0; y2_ = rect().height();
        emit resized();
        status_ = CAPTURED;
    });
}

void Selector::setBorderColor(const QColor &c)
{
    border_color_ = c;
}

void Selector::setBorderWidth(int w)
{
    border_width_ = w;
}

void Selector::setBorderStyle(Qt::PenStyle s)
{
    border_style_ = s;
}

void Selector::setMaskColor(const QColor& c)
{
    mask_color_ = c;
}

void Selector::setUseDetectWindow(bool f)
{
    use_detect_ = f;
}
