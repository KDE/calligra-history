/*
 *
 */

#ifndef KODOCKWONTAINER_H
#define KODOCKWONTAINER_H
#include <QWidget>

class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDockWidget;
class QEvent;
class QObjet;

class KoDockContainer : public QWidget
{
Q_OBJECT
public :
    KoDockContainer();
    void catchDockWidget(QDockWidget *widget);
    void releaseDockWidget(QDockWidget *widget);
    void addDockWidget(QDockWidget *widget);

protected :
    bool event(QEvent* event);
    void dropEvent(QDropEvent * event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);

    bool isOver(const QPoint& pos );
    bool eventFilter(QObject *object, QEvent *event);

private :
    QList<QDockWidget*> m_widgets;
    QList<Qt::WindowFlags> m_widgetsFlags;
};
#endif /*KODOCKWONTAINER_H*/

