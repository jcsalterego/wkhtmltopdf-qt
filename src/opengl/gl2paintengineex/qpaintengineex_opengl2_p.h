/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGRAPHICSCONTEXT_OPENGL2_P_H
#define QGRAPHICSCONTEXT_OPENGL2_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QDebug>

#include <private/qpaintengineex_p.h>
#include <private/qglengineshadermanager_p.h>
#include <private/qgl2pexvertexarray_p.h>
#include <private/qglpaintdevice_p.h>
#include <private/qglpixmapfilter_p.h>
#include <private/qfontengine_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qtriangulatingstroker_p.h>

enum EngineMode {
    ImageDrawingMode,
    TextDrawingMode,
    BrushDrawingMode,
    ImageArrayDrawingMode
};

QT_BEGIN_NAMESPACE

#define GL_STENCIL_HIGH_BIT         GLuint(0x80)
#define QT_BRUSH_TEXTURE_UNIT       GLuint(0)
#define QT_IMAGE_TEXTURE_UNIT       GLuint(0) //Can be the same as brush texture unit
#define QT_MASK_TEXTURE_UNIT        GLuint(1)
#define QT_BACKGROUND_TEXTURE_UNIT  GLuint(2)

class QGL2PaintEngineExPrivate;


class QOpenGL2PaintEngineState : public QPainterState
{
public:
    QOpenGL2PaintEngineState(QOpenGL2PaintEngineState &other);
    QOpenGL2PaintEngineState();
    ~QOpenGL2PaintEngineState();

    uint isNew : 1;
    uint needsClipBufferClear : 1;
    uint clipTestEnabled : 1;
    uint canRestoreClip : 1;
    uint matrixChanged : 1;
    uint compositionModeChanged : 1;
    uint opacityChanged : 1;
    uint renderHintsChanged : 1;
    uint clipChanged : 1;
    uint currentClip : 8;

    QRect rectangleClip;
};

class Q_OPENGL_EXPORT QGL2PaintEngineEx : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QGL2PaintEngineEx)
public:
    QGL2PaintEngineEx();
    ~QGL2PaintEngineEx();

    bool begin(QPaintDevice *device);
    void ensureActive();
    bool end();

    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void brushChanged();
    virtual void brushOriginChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();

    virtual void drawTexture(const QRectF &r, GLuint textureId, const QSize &size, const QRectF &sr);
    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    virtual void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                                     QPainter::PixmapFragmentHints hints);
    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                           Qt::ImageConversionFlags flags = Qt::AutoColor);
    virtual void drawTextItem(const QPointF &p, const QTextItem &textItem);
    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void stroke(const QVectorPath &path, const QPen &pen);
    virtual void clip(const QVectorPath &path, Qt::ClipOperation op);

    virtual void drawStaticTextItem(QStaticTextItem *textItem);

    Type type() const { return OpenGL2; }

    virtual void setState(QPainterState *s);
    virtual QPainterState *createState(QPainterState *orig) const;
    inline QOpenGL2PaintEngineState *state() {
        return static_cast<QOpenGL2PaintEngineState *>(QPaintEngineEx::state());
    }
    inline const QOpenGL2PaintEngineState *state() const {
        return static_cast<const QOpenGL2PaintEngineState *>(QPaintEngineEx::state());
    }

    void beginNativePainting();
    void endNativePainting();

    QPixmapFilter *pixmapFilter(int type, const QPixmapFilter *prototype);

    void setRenderTextActive(bool);

    bool isNativePaintingActive() const;
private:
    Q_DISABLE_COPY(QGL2PaintEngineEx)
};

class QGL2PaintEngineExPrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QGL2PaintEngineEx)
public:
    enum StencilFillMode {
        OddEvenFillMode,
        WindingFillMode,
        TriStripStrokeFillMode
    };

    QGL2PaintEngineExPrivate(QGL2PaintEngineEx *q_ptr) :
            q(q_ptr),
            shaderManager(0),
            width(0), height(0),
            ctx(0),
            useSystemClip(true),
            elementIndicesVBOId(0),
            snapToPixelGrid(false),
            addOffset(false),
            nativePaintingActive(false),
            inverseScale(1),
            lastMaskTextureUsed(0)
    { }

    ~QGL2PaintEngineExPrivate();

    void updateBrushTexture();
    void updateBrushUniforms();
    void updateMatrix();
    void updateCompositionMode();
    void updateTextureFilter(GLenum target, GLenum wrapMode, bool smoothPixmapTransform, GLuint id = -1);

    void resetGLState();

    // fill, stroke, drawTexture, drawPixmaps & drawCachedGlyphs are the main rendering entry-points,
    // however writeClip can also be thought of as en entry point as it does similar things.
    void fill(const QVectorPath &path);
    void stroke(const QVectorPath &path, const QPen &pen);
    void drawTexture(const QGLRect& dest, const QGLRect& src, const QSize &textureSize, bool opaque, bool pattern = false);
    void drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                             QPainter::PixmapFragmentHints hints);
    void drawCachedGlyphs(QFontEngineGlyphCache::Type glyphType, QStaticTextItem *staticTextItem,
                          bool includeMatrixInCache);

    // Calls glVertexAttributePointer if the pointer has changed
    inline void setVertexAttributePointer(unsigned int arrayIndex, const GLfloat *pointer);

    // draws whatever is in the vertex array:
    void drawVertexArrays(const float *data, int *stops, int stopCount, GLenum primitive);
    void drawVertexArrays(QGL2PEXVertexArray &vertexArray, GLenum primitive) {
        drawVertexArrays((const float *) vertexArray.data(), vertexArray.stops(), vertexArray.stopCount(), primitive);
    }

    // Composites the bounding rect onto dest buffer:
    void composite(const QGLRect& boundingRect);

    // Calls drawVertexArrays to render into stencil buffer:
    void fillStencilWithVertexArray(const float *data, int count, int *stops, int stopCount, const QGLRect &bounds, StencilFillMode mode);
    void fillStencilWithVertexArray(QGL2PEXVertexArray& vertexArray, bool useWindingFill) {
        fillStencilWithVertexArray((const float *) vertexArray.data(), 0, vertexArray.stops(), vertexArray.stopCount(),
                                   vertexArray.boundingRect(),
                                   useWindingFill ? WindingFillMode : OddEvenFillMode);
    }

    void setBrush(const QBrush& brush);
    void transferMode(EngineMode newMode);
    bool prepareForDraw(bool srcPixelsAreOpaque); // returns true if the program has changed
    inline void useSimpleShader();
    inline GLuint location(const QGLEngineShaderManager::Uniform uniform) {
        return shaderManager->getUniformLocation(uniform);
    }

    void clearClip(uint value);
    void writeClip(const QVectorPath &path, uint value);
    void resetClipIfNeeded();

    void updateClipScissorTest();
    void setScissor(const QRect &rect);
    void regenerateClip();
    void systemStateChanged();


    static QGLEngineShaderManager* shaderManagerForEngine(QGL2PaintEngineEx *engine) { return engine->d_func()->shaderManager; }
    static QGL2PaintEngineExPrivate *getData(QGL2PaintEngineEx *engine) { return engine->d_func(); }
    static void cleanupVectorPath(QPaintEngineEx *engine, void *data);


    QGL2PaintEngineEx* q;
    QGLEngineShaderManager* shaderManager;
    QGLPaintDevice* device;
    int width, height;
    QGLContext *ctx;
    EngineMode mode;
    QFontEngineGlyphCache::Type glyphCacheType;

    // Dirty flags
    bool matrixDirty; // Implies matrix uniforms are also dirty
    bool compositionModeDirty;
    bool brushTextureDirty;
    bool brushUniformsDirty;
    bool opacityUniformDirty;
    bool matrixUniformDirty;

    bool stencilClean; // Has the stencil not been used for clipping so far?
    bool useSystemClip;
    QRegion dirtyStencilRegion;
    QRect currentScissorBounds;
    uint maxClip;

    QBrush currentBrush; // May not be the state's brush!
    const QBrush noBrush;

    QGL2PEXVertexArray vertexCoordinateArray;
    QGL2PEXVertexArray textureCoordinateArray;
    QVector<GLushort> elementIndices;
    GLuint elementIndicesVBOId;
    QDataBuffer<GLfloat> opacityArray;
    GLfloat staticVertexCoordinateArray[8];
    GLfloat staticTextureCoordinateArray[8];

    bool snapToPixelGrid;
    bool addOffset; // When enabled, adds a 0.49,0.49 offset to matrix in updateMatrix
    bool nativePaintingActive;
    GLfloat pmvMatrix[3][3];
    GLfloat inverseScale;

    GLuint lastTextureUsed;
    GLuint lastMaskTextureUsed;

    bool needsSync;
    bool multisamplingAlwaysEnabled;

    GLfloat depthRange[2];

    float textureInvertedY;

    QTriangulatingStroker stroker;
    QDashedStrokeProcessor dasher;

    QScopedPointer<QPixmapFilter> convolutionFilter;
    QScopedPointer<QPixmapFilter> colorizeFilter;
    QScopedPointer<QPixmapFilter> blurFilter;
    QScopedPointer<QPixmapFilter> dropShadowFilter;

    QSet<QVectorPath::CacheEntry *> pathCaches;
    QVector<GLuint> unusedVBOSToClean;
    QVector<GLuint> unusedIBOSToClean;

    const GLfloat *vertexAttribPointers[3];
};


void QGL2PaintEngineExPrivate::setVertexAttributePointer(unsigned int arrayIndex, const GLfloat *pointer)
{
    Q_ASSERT(arrayIndex < 3);
    if (pointer == vertexAttribPointers[arrayIndex])
        return;

    vertexAttribPointers[arrayIndex] = pointer;
    if (arrayIndex == QT_OPACITY_ATTR)
        glVertexAttribPointer(arrayIndex, 1, GL_FLOAT, GL_FALSE, 0, pointer);
    else
        glVertexAttribPointer(arrayIndex, 2, GL_FLOAT, GL_FALSE, 0, pointer);
}

QT_END_NAMESPACE

#endif
