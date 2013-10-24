/**
 * Copyright (c) 2012 Nokia Corporation.
 */

#include "pagerenderer.h"

using namespace BE3D;


// NOTE, update shadow to second power! -> infinite

const char* strPageFragmentShader =
    "uniform sampler2D sampler2d;\n"
    "varying mediump vec2 texCoord;\n"
    "varying mediump vec3 pixelnormal;\n"
    "uniform mediump vec4 dividermat;\n"
    "uniform mediump float shadowpower;\n"
    "void main (void)\n"
    "{\n"
        "   mediump float light=0.4 + dot( normalize(pixelnormal), normalize(vec3( 0.0,0.0,-1.0 )) )*0.6;\n"
        "   mediump float shadowmul=3.0*((texCoord.x-dividermat.x)*dividermat.z + ((1.0-texCoord.y)-dividermat.y)*dividermat.w);\n"
        "   shadowmul = 1.0-(1.0/(1.0+shadowmul*shadowmul))*shadowpower;\n"
        "   light*=shadowmul;\n"
        "    gl_FragColor = texture2D(sampler2d, texCoord) * vec4(light,light,light,1.0);\n"
    "}";


const char* strPageVertexShader =
    "attribute highp vec3 vertex;\n"
    "attribute highp vec2 uv;\n"
    "attribute highp vec3 normal;\n"

    "uniform mediump mat4 projMatrix;\n"
    "uniform mediump float backsidemul;\n"
    "varying mediump vec2 texCoord;\n"
    "varying mediump vec3 pixelnormal;\n"
    "void main(void)\n"
    "{\n"
        "gl_Position = projMatrix * vec4( vertex,1 );\n"
        "texCoord = vec2( uv.x * backsidemul, uv.y );\n"
        "pixelnormal = normal * backsidemul;\n"
    "}";


PageRenderer::PageRenderer( int gw, int gh, QObject *parent) : QObject( parent ) {
    qtibo = 0;
    gridWidth = gw;
    gridHeight = gh;
}


PageRenderer::~PageRenderer() {

}


void PageRenderer::initializeGL() {
    destroyGL();

    // Create the program
    QGLShader *vshader = new QGLShader(QGLShader::Vertex, this);
    if (vshader->compileSourceCode( strPageVertexShader ) == false)
        qDebug() << "Page: Failed to compile vertexshader.";
    QGLShader *fshader = new QGLShader(QGLShader::Fragment, this);
    if (fshader->compileSourceCode( strPageFragmentShader ) == false)
        qDebug() << "Page: Failed to compile fragmentshader.";

    program.addShader( vshader );
    program.addShader( fshader );

    program.bindAttributeLocation( "vertex", 0 );
    program.bindAttributeLocation( "uv", 1 );
    program.bindAttributeLocation( "normal", 2 );

    if (program.link() == false)
        qDebug() << "Page: Error linking the shaderprogram.";
    else
        qDebug() << "Page: Building the shaderprogram finished.";


    shadowPowerLocation = program.uniformLocation("shadowpower");
    dividerMatLocation = program.uniformLocation("dividermat");
    samplerLocation = program.uniformLocation("sampler2d");
    projmLocation = program.uniformLocation("projMatrix");



    // Create the indexbuffer
    qtibo = new QGLBuffer( QGLBuffer::IndexBuffer );
    if (qtibo->create() == false)
        qDebug() << "BPage: Failed to create IBO.";
    else
        qDebug() << "BPage: Created the IBO.";

    indexCount = (gridWidth-1) * (gridHeight-1 ) * 3 * 2;
    GLushort *tempIndices = new GLushort[ indexCount ];

    // place indicces

    GLushort *t = tempIndices;
    for (int y=0; y<gridHeight-1; y++)
        for (int x=0; x<gridWidth-1; x++) {
            t[0] = y*gridWidth+x;
            t[1] = y*gridWidth+x+1;
            t[2] = (y+1)*gridWidth+x;
            t+=3;
            t[0] = (y+1)*gridWidth+x;;
            t[1] = y*gridWidth+x+1;
            t[2] = (y+1)*gridWidth+x+1;
            t+=3;
        }


    qtibo->bind();
    qtibo->allocate( tempIndices, indexCount*sizeof(GLushort) );

    delete [] tempIndices;
}


void PageRenderer::destroyGL() {
    program.release();
    if (qtibo) {
        if (qtibo) delete qtibo;
        qtibo = 0;
    }
}


