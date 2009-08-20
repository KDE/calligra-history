/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "opengl/kis_opengl_canvas2.h"


#ifdef HAVE_OPENGL

#include <QMenu>
#include <QWidget>
#include <QGLWidget>
#include <QGLContext>
#include <QImage>
#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>

#include <kxmlguifactory.h>

#include "KoToolProxy.h"
#include "KoToolManager.h"
#include "KoColorSpace.h"
#include "KoShapeManager.h"

#include "kis_types.h"
#include "canvas/kis_canvas2.h"
#include "kis_image.h"
#include "opengl/kis_opengl.h"
#include "opengl/kis_opengl_image_textures.h"
#include "kis_view2.h"
#include "kis_canvas_resource_provider.h"
#include "kis_config.h"
#include "kis_debug.h"
#include "kis_selection_manager.h"

class KisOpenGLCanvas2::Private
{
public:
    Private(const KoViewConverter *vc)
            : viewConverter(vc)
            , canvas(0)
            , toolProxy(0)
            , previousEvent(QEvent::TabletRelease, QPoint(), QPoint(), QPointF(),
                            QTabletEvent::NoDevice, 0, 0.0, 0, 0, 0, 0, 0, Qt::NoModifier, Q_INT64_C(932838457459459)) {}
    const KoViewConverter * viewConverter;
    KisCanvas2 * canvas;
    KoToolProxy * toolProxy;
    KisOpenGLImageTexturesSP openGLImageTextures;
    QPoint documentOffset;
    bool tabletDown;
    QTabletEvent previousEvent;

};

KisOpenGLCanvas2::KisOpenGLCanvas2(KisCanvas2 * canvas, QWidget * parent, KisOpenGLImageTexturesSP imageTextures)
        : QGLWidget(QGLFormat(QGL::SampleBuffers), parent, KisOpenGL::sharedContextWidget())
        , m_d(new Private(canvas->viewConverter()))
{
    m_d->canvas = canvas;
    m_d->toolProxy = canvas->toolProxy();
    m_d->openGLImageTextures = imageTextures;

    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_NoSystemBackground);
    imageTextures->generateBackgroundTexture(checkImage(KisOpenGLImageTextures::BACKGROUND_TEXTURE_CHECK_SIZE));
    setAttribute(Qt::WA_InputMethodEnabled, true);

    if (isSharing()) {
        dbgUI << "Created QGLWidget with sharing";
    } else {
        dbgUI << "Created QGLWidget with no sharing";
    }
}

KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
    delete m_d;
}

void KisOpenGLCanvas2::initializeGL()
{
    qglClearColor(QColor::fromCmykF(0.40, 0.0, 1.0, 0.0));
    glShadeModel(GL_FLAT);
    //glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
}

void KisOpenGLCanvas2::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
    updateGL();
}

void KisOpenGLCanvas2::paintGL()
{
    QColor widgetBackgroundColor = palette().color(QPalette::Mid);

    glClearColor(widgetBackgroundColor.red() / 255.0, widgetBackgroundColor.green() / 255.0, widgetBackgroundColor.blue() / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    KisImageSP img = m_d->canvas->image();

    if (!img) return;

    // Zoom factor
    qreal sx, sy;
    m_d->viewConverter->zoom(&sx, &sy);

    // Resolution
    qreal pppx, pppy;
    pppx = img->xRes();
    pppy = img->yRes();

    // Compute the scale factors
    qreal scaleX = sx / pppx;
    qreal scaleY = sy / pppy;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width(), height());
    glOrtho(0, width(), height(), 0, -1, 1);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    KisConfig cfg;
    GLfloat checkSizeScale = KisOpenGLImageTextures::BACKGROUND_TEXTURE_CHECK_SIZE / static_cast<GLfloat>(cfg.checkSize());

    glScalef(checkSizeScale, checkSizeScale, 1.0);

    if (cfg.scrollCheckers()) {
        glTranslatef(static_cast<GLfloat>(m_d->documentOffset.x()) / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE,
                     static_cast<GLfloat>(m_d->documentOffset.y()) / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE,
                     0.0);
    }

    glScalef(scaleX, scaleY, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(scaleX, scaleY, 1.0);

    glBindTexture(GL_TEXTURE_2D, m_d->openGLImageTextures->backgroundTexture());

    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0, 0.0);
    glVertex2f(0.0, 0.0);

    glTexCoord2f(img->width() / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE, 0.0);
    glVertex2f(img->width(), 0.0);

    glTexCoord2f(img->width() / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE,
                 img->height() / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE);
    glVertex2f(img->width(), img->height());

    glTexCoord2f(0.0, img->height() / KisOpenGLImageTextures::BACKGROUND_TEXTURE_SIZE);
    glVertex2f(0.0, img->height());

    glEnd();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-m_d->documentOffset.x(), -m_d->documentOffset.y(), 0.0);
    glScalef(scaleX, scaleY, 1.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    QRectF documentRect = m_d->viewConverter->viewToDocument(QRectF(m_d->documentOffset.x(), m_d->documentOffset.y(), width(), height()));
    QRect wr = img->documentToIntPixel(documentRect);
    wr &= QRect(0, 0, img->width(), img->height());

    if (img->colorSpace()->hasHighDynamicRange()) {
        if (m_d->openGLImageTextures->usingHDRExposureProgram()) {
            m_d->openGLImageTextures->activateHDRExposureProgram();
        }
        m_d->openGLImageTextures->setHDRExposure(m_d->canvas->view()->resourceProvider()->HDRExposure());
    }

    makeCurrent();

    for (int x = (wr.left() / m_d->openGLImageTextures->imageTextureTileWidth()) * m_d->openGLImageTextures->imageTextureTileWidth();
            x <= wr.right();
            x += m_d->openGLImageTextures->imageTextureTileWidth()) {
        for (int y = (wr.top() / m_d->openGLImageTextures->imageTextureTileHeight()) * m_d->openGLImageTextures->imageTextureTileHeight();
                y <= wr.bottom();
                y += m_d->openGLImageTextures->imageTextureTileHeight()) {

            glBindTexture(GL_TEXTURE_2D, m_d->openGLImageTextures->imageTextureTile(x, y));

            glBegin(GL_QUADS);

            glTexCoord2f(0.0, 0.0);
            glVertex2f(x, y);

            glTexCoord2f(1.0, 0.0);
            glVertex2f(x + m_d->openGLImageTextures->imageTextureTileWidth(), y);

            glTexCoord2f(1.0, 1.0);
            glVertex2f(x + m_d->openGLImageTextures->imageTextureTileWidth(), y + m_d->openGLImageTextures->imageTextureTileHeight());

            glTexCoord2f(0.0, 1.0);
            glVertex2f(x, y + m_d->openGLImageTextures->imageTextureTileHeight());

            glEnd();
        }
    }

    if (img->colorSpace()->hasHighDynamicRange()) {
        if (m_d->openGLImageTextures->usingHDRExposureProgram()) {
            m_d->openGLImageTextures->deactivateHDRExposureProgram();
        }
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Unbind the texture otherwise the ATI driver crashes when the canvas context is
    // made current after the textures are deleted following an image resize.
    glBindTexture(GL_TEXTURE_2D, 0);


    // XXX: make settable
    bool drawTools = true;

    QPainter gc(this);

    drawDecorations(gc, drawTools,
                    m_d->documentOffset,
                    QRect(QPoint(0, 0), QSize()),
                    m_d->canvas);

    gc.end();
}

void KisOpenGLCanvas2::setPixelToViewTransformation(void)
{
    KisImageSP img = m_d->canvas->image();

    if (!img) return;

    // Zoom factor
    qreal sx, sy;
    m_d->viewConverter->zoom(&sx, &sy);

    // Resolution
    qreal pppx, pppy;
    pppx = img->xRes();
    pppy = img->yRes();

    // Compute the scale factors
    qreal scaleX = sx / pppx;
    qreal scaleY = sy / pppy;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width(), height());
    glOrtho(0, width(), height(), 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-m_d->documentOffset.x(), -m_d->documentOffset.y(), 0.0);
    glScalef(scaleX, scaleY, 1.0);
}

void KisOpenGLCanvas2::enterEvent(QEvent* e)
{
    dbgRender << "Enter event ";
    QWidget::enterEvent(e);
}

void KisOpenGLCanvas2::leaveEvent(QEvent* e)
{
    dbgRender << "Leave event ";
    if (m_d->tabletDown) {
        m_d->tabletDown = false;
        QTabletEvent* fakeEvent = new QTabletEvent(QEvent::TabletRelease, m_d->previousEvent.pos(), m_d->previousEvent.globalPos(), m_d->previousEvent.hiResGlobalPos(), m_d->previousEvent.device(), m_d->previousEvent.pointerType(), m_d->previousEvent.pressure(), m_d->previousEvent.xTilt(), m_d->previousEvent.yTilt(), m_d->previousEvent.tangentialPressure(), m_d->previousEvent.rotation(), m_d->previousEvent.z(), m_d->previousEvent.modifiers(), m_d->previousEvent.uniqueId());
        m_d->toolProxy->tabletEvent(fakeEvent , QPointF());   // HACK this fake event is a work around a bug in Qt which stops sending tablet events when the tablet pen move outside the widget (and you get a nasty surprise when the cursor moves back on the widget especially if you have released your tablet as krita is still in drawing mode)
    }
    QWidget::leaveEvent(e);
}


void KisOpenGLCanvas2::mouseMoveEvent(QMouseEvent *e)
{
    m_d->toolProxy->mouseMoveEvent(e, m_d->viewConverter->viewToDocument(e->pos() + m_d->documentOffset));
}

void KisOpenGLCanvas2::contextMenuEvent(QContextMenuEvent *e)
{
    m_d->canvas->view()->unplugActionList("flake_tool_actions");
    m_d->canvas->view()->plugActionList("flake_tool_actions",
                                        m_d->toolProxy->popupActionList());
    QMenu *menu = dynamic_cast<QMenu*>(m_d->canvas->view()->factory()->container("image_popup", m_d->canvas->view()));
    if (menu)
        menu->exec(e->globalPos());
}

void KisOpenGLCanvas2::mousePressEvent(QMouseEvent *e)
{
    m_d->toolProxy->mousePressEvent(e, m_d->viewConverter->viewToDocument(e->pos() + m_d->documentOffset));
}

void KisOpenGLCanvas2::mouseReleaseEvent(QMouseEvent *e)
{
    m_d->toolProxy->mouseReleaseEvent(e, m_d->viewConverter->viewToDocument(e->pos() + m_d->documentOffset));
}

void KisOpenGLCanvas2::mouseDoubleClickEvent(QMouseEvent *e)
{
    m_d->toolProxy->mouseDoubleClickEvent(e, m_d->viewConverter->viewToDocument(e->pos() + m_d->documentOffset));
}

void KisOpenGLCanvas2::keyPressEvent(QKeyEvent *e)
{
    m_d->toolProxy->keyPressEvent(e);
    if (! e->isAccepted()) {
        if (e->key() == Qt::Key_Backtab
                || (e->key() == Qt::Key_Tab && (e->modifiers() & Qt::ShiftModifier)))
            focusNextPrevChild(false);
        else if (e->key() == Qt::Key_Tab)
            focusNextPrevChild(true);
    }
}

void KisOpenGLCanvas2::keyReleaseEvent(QKeyEvent *e)
{
    m_d->toolProxy->keyReleaseEvent(e);
}

QVariant KisOpenGLCanvas2::inputMethodQuery(Qt::InputMethodQuery query) const
{
    return m_d->toolProxy->inputMethodQuery(query, *m_d->viewConverter);
}

void KisOpenGLCanvas2::inputMethodEvent(QInputMethodEvent *event)
{
    m_d->toolProxy->inputMethodEvent(event);
}

void KisOpenGLCanvas2::tabletEvent(QTabletEvent *e)
{
    dbgRender << "tablet event:" << e->pressure() << e->type() << " " << e->device();
    switch (e->type()) {
    case QEvent::TabletPress:
        m_d->tabletDown = true;
        break;
    case QEvent::TabletRelease:
        m_d->tabletDown = false;
        break;
    case QEvent::TabletMove:
        break;
    default:
        ; // ignore the rest.
    }
    qreal subpixelX = e->hiResGlobalX();
    subpixelX = subpixelX - ((int) subpixelX); // leave only part behind the dot
    qreal subpixelY = e->hiResGlobalY();
    subpixelY = subpixelY - ((int) subpixelY); // leave only part behind the dot
    QPointF pos(e->x() + subpixelX + m_d->documentOffset.x(), e->y() + subpixelY + m_d->documentOffset.y());

    m_d->previousEvent = *e;
    m_d->toolProxy->tabletEvent(e, m_d->viewConverter->viewToDocument(pos));
}

void KisOpenGLCanvas2::wheelEvent(QWheelEvent *e)
{
    m_d->toolProxy->wheelEvent(e, m_d->viewConverter->viewToDocument(e->pos() + m_d->documentOffset));
}

KoToolProxy * KisOpenGLCanvas2::toolProxy()
{
    return m_d->toolProxy;
}

void KisOpenGLCanvas2::documentOffsetMoved(const QPoint & pt)
{
    m_d->documentOffset = pt;
    updateGL();
}


#include "kis_opengl_canvas2.moc"
#endif // HAVE_OPENGL
