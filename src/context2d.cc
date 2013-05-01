#include <node.h>
#include <node_buffer.h>

#include "context2d.h"
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkPath.h>
#include <SkError.h>

#include <SkStream.h>
#include <SkDevice.h>
#include <SkBlurMaskFilter.h>
#include <SkData.h>
#include <SkGraphics.h>
#include <SkGradientShader.h>
#include <SkShader.h>
#include <SkImageEncoder.h>
#include <SkRect.h>
#include <SkRegion.h>
#include <SkTypeface.h>
#include <SkMatrix44.h>
#include <SkXfermode.h>
#include <SkBitmapProcShader.h>

#include <stdio.h>
#include <assert.h>

#define TAU M_PI*2
using namespace node;
using namespace v8;

#define PROTOTYPE_METHOD(name, exportName) \
  tpl->PrototypeTemplate()->Set(String::NewSymbol(#exportName), \
       FunctionTemplate::New(name)->GetFunction()); \

#define METHOD(name) Handle<Value> Context2D::name(const Arguments& args)
#define DEGREES(rads) rads * 180/M_PI

void Context2D::Init(v8::Handle<v8::Object> exports) {
  SkAutoGraphics ag;

  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("Context2D"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Non-standard
  PROTOTYPE_METHOD(ToPngBuffer, toPngBuffer);
  PROTOTYPE_METHOD(DumpState, dumpState);
  PROTOTYPE_METHOD(ToBuffer, toBuffer);
  PROTOTYPE_METHOD(GetPixel, getPixel);
  PROTOTYPE_METHOD(Resize, resize);


  // Standard
  PROTOTYPE_METHOD(Save, save);
  PROTOTYPE_METHOD(Restore, restore);
  PROTOTYPE_METHOD(Scale, scale);
  PROTOTYPE_METHOD(Rotate, rotate);
  PROTOTYPE_METHOD(Translate, translate);
  PROTOTYPE_METHOD(Transform, transform);
  PROTOTYPE_METHOD(ResetMatrix, resetMatrix);
  PROTOTYPE_METHOD(SetGlobalAlpha, setGlobalAlpha);
  PROTOTYPE_METHOD(GetGlobalAlpha, getGlobalAlpha);
  PROTOTYPE_METHOD(SetGlobalCompositeOperation, setGlobalCompositeOperation);
  PROTOTYPE_METHOD(GetGlobalCompositeOperation, getGlobalCompositeOperation);
  PROTOTYPE_METHOD(SetImageSmoothingEnabled, setImageSmoothingEnabled);
  PROTOTYPE_METHOD(GetImageSmoothingEnabled, getImageSmoothingEnabled);
  PROTOTYPE_METHOD(SetStrokeStyle, setStrokeStyle);
  PROTOTYPE_METHOD(GetStrokeStyle, getStrokeStyle);
  PROTOTYPE_METHOD(SetFillStylePattern, setFillStylePattern);
  PROTOTYPE_METHOD(SetFillStyle, setFillStyle);
  PROTOTYPE_METHOD(GetFillStyle, getFillStyle);
  PROTOTYPE_METHOD(SetLinearGradientShader, setLinearGradientShader);
  PROTOTYPE_METHOD(SetRadialGradientShader, setRadialGradientShader);
  PROTOTYPE_METHOD(SetShadowOffsetX, setShadowOffsetX);
  PROTOTYPE_METHOD(GetShadowOffsetX, getShadowOffsetX);
  PROTOTYPE_METHOD(SetShadowOffsetY, setShadowOffsetY);
  PROTOTYPE_METHOD(GetShadowOffsetY, getShadowOffsetY);
  PROTOTYPE_METHOD(SetShadowBlur, setShadowBlur);
  PROTOTYPE_METHOD(GetShadowBlur, getShadowBlur);
  PROTOTYPE_METHOD(SetShadowColor, setShadowColor);
  PROTOTYPE_METHOD(GetShadowColor, getShadowColor);
  PROTOTYPE_METHOD(ClearRect, clearRect);
  PROTOTYPE_METHOD(FillRect, fillRect);
  PROTOTYPE_METHOD(StrokeRect, strokeRect);
  PROTOTYPE_METHOD(BeginPath, beginPath);
  PROTOTYPE_METHOD(Fill, fill);
  PROTOTYPE_METHOD(Stroke, stroke);
  PROTOTYPE_METHOD(Clip, clip);
  PROTOTYPE_METHOD(IsPointInPath, isPointInPath);
  PROTOTYPE_METHOD(ClosePath, closePath);
  PROTOTYPE_METHOD(MoveTo, moveTo);
  PROTOTYPE_METHOD(LineTo, lineTo);
  PROTOTYPE_METHOD(QuadraticCurveTo, quadraticCurveTo);
  PROTOTYPE_METHOD(BezierCurveTo, bezierCurveTo);
  PROTOTYPE_METHOD(ArcTo, arcTo);
  PROTOTYPE_METHOD(Rect, rect);
  PROTOTYPE_METHOD(Arc, arc);
  PROTOTYPE_METHOD(Ellipse, ellipse);
  PROTOTYPE_METHOD(FillText, fillText);
  PROTOTYPE_METHOD(StrokeText, strokeText);
  PROTOTYPE_METHOD(MeasureText, measureText);
  PROTOTYPE_METHOD(GetFont, getFont);
  PROTOTYPE_METHOD(SetFont, setFont);
  PROTOTYPE_METHOD(GetTextAlign, getTextAlign);
  PROTOTYPE_METHOD(SetTextAlign, setTextAlign);
  PROTOTYPE_METHOD(GetTextBaseline, getTextBaseline);
  PROTOTYPE_METHOD(SetTextBaseline, setTextBaseline);
  PROTOTYPE_METHOD(DrawImageBuffer, drawImageBuffer);
  PROTOTYPE_METHOD(CreateImageData, createImageData);
  PROTOTYPE_METHOD(GetImageData, getImageData);
  PROTOTYPE_METHOD(PutImageData, putImageData);
  PROTOTYPE_METHOD(GetLineWidth, getLineWidth);
  PROTOTYPE_METHOD(SetLineWidth, setLineWidth);
  PROTOTYPE_METHOD(SetLineCap, setLineCap);
  PROTOTYPE_METHOD(SetLineJoin, setLineJoin);
  PROTOTYPE_METHOD(GetMiterLimit, getMiterLimit);
  PROTOTYPE_METHOD(SetMiterLimit, setMiterLimit);
  PROTOTYPE_METHOD(SetLineDash, setLineDash);
  PROTOTYPE_METHOD(GetLineDash, getLineDash);
  PROTOTYPE_METHOD(SetLineDashOffset, setLineDashOffset);
  PROTOTYPE_METHOD(GetLineDashOffset, getLineDashOffset);

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  exports->Set(String::NewSymbol("Context2D"), constructor);

}

Context2D::Context2D(int w, int h) {
  SkBitmap bitmap;

  bitmap.setConfig(SkBitmap::kARGB_8888_Config, w, h);
  bitmap.allocPixels();

  this->canvas = new SkCanvas(bitmap);
  this->canvas->clear(SkColorSetARGBInline(0, 0, 0, 0));

  this->globalAlpha = 255;
  this->globalCompositeOperation = SkXfermode::kSrcOver_Mode;

  this->paint.setXfermodeMode(this->globalCompositeOperation);
  this->paint.setColor(SK_ColorBLACK);

  this->shadowX = 0;
  this->shadowY = 0;
  this->shadowBlur = 0;
  this->shadowPaint.setColor(0x00000000);
}

Context2D::~Context2D() {
  this->canvas->unref();
}

METHOD(New) {
  HandleScope scope;

  int w = args[0]->NumberValue();
  int h = args[1]->NumberValue();

  Context2D* context = new Context2D(w, h);
  context->Wrap(args.This());

  return args.This();
}

METHOD(Resize) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  int width = args[0]->Uint32Value() & 0xff;
  int height = args[1]->Uint32Value() & 0xff;

  SkDevice *device = ctx->canvas->createCompatibleDevice(
    SkBitmap::kARGB_8888_Config,
    width,
    height,
    false
  );

  // TODO: check for memory leakage.
  SkSafeUnref(ctx->canvas);
  ctx->canvas = new SkCanvas(device);

  return scope.Close(Undefined());
}

METHOD(DumpState) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  SkMatrix44 matrix(ctx->canvas->getTotalMatrix());
  matrix.dump();
  ctx->path.dump();

  return scope.Close(Undefined());
}

METHOD(GetPixel) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  Local<Object> obj = Object::New();

  SkBitmap bitmap = ctx->canvas->getDevice()->accessBitmap(false);

  ctx->canvas->flush();

  bitmap.lockPixels();

  // TODO: validity and bounds
  SkColor color = bitmap.getColor(
    args[0]->NumberValue(),
    args[1]->NumberValue()
  );

  bitmap.unlockPixels();

  obj->Set(String::NewSymbol("r"), Number::New(SkColorGetR(color)));
  obj->Set(String::NewSymbol("g"), Number::New(SkColorGetG(color)));
  obj->Set(String::NewSymbol("b"), Number::New(SkColorGetB(color)));
  obj->Set(String::NewSymbol("a"), Number::New(SkColorGetA(color)));



  return scope.Close(obj);
}

METHOD(ToPngBuffer) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  ctx->canvas->flush();
  SkBitmap bitmap = ctx->canvas->getDevice()->accessBitmap(false);
  size_t size = bitmap.getSize();

  void *data = malloc(size);
  SkMemoryWStream stream(data, size);
  SkImageEncoder::EncodeStream(&stream, bitmap, SkImageEncoder::kPNG_Type, 100);

  Buffer *buffer = Buffer::New((const char *)data, size);

  Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
  Local<Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
  Handle<Value> constructorArgs[3] = { buffer->handle_, v8::Integer::New(Buffer::Length(buffer)), v8::Integer::New(0) };
  Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);

  free(data);

  return scope.Close(actualBuffer);
}


METHOD(ToBuffer) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());


  SkBitmap bitmap = ctx->canvas->getDevice()->accessBitmap(true);
  size_t size = bitmap.getSize();

  ctx->canvas->flush();
  bitmap.lockPixels();

  Buffer *buffer = Buffer::New(size);
  memcpy(Buffer::Data(buffer), (const char *)bitmap.getPixels(), size);
  bitmap.unlockPixels();

  Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
  Local<Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
  Handle<Value> constructorArgs[3] = { buffer->handle_, v8::Integer::New(Buffer::Length(buffer)), v8::Integer::New(0) };
  Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);

  return scope.Close(actualBuffer);
}


METHOD(Save) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  ctx->canvas->save();

  return scope.Close(Undefined());
}

METHOD(Restore) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  ctx->canvas->restore();

  return scope.Close(Undefined());
}

METHOD(Scale) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  if (!args[0]->IsUndefined() && !args[1]->IsUndefined()) {
    ctx->canvas->scale(
      SkDoubleToScalar(args[0]->NumberValue()),
      SkDoubleToScalar(args[1]->NumberValue())
    );
  }

  return scope.Close(Undefined());
}

METHOD(Rotate) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  if (!args[0]->IsUndefined()) {
    SkScalar rads = SkDoubleToScalar(args[0]->NumberValue());
    ctx->canvas->rotate(DEGREES(rads));
  }

  return scope.Close(Undefined());
}

METHOD(Translate) {
  HandleScope scope;
  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  if (!args[0]->IsUndefined() && !args[0]->IsUndefined()) {
    SkScalar x = SkDoubleToScalar(args[0]->NumberValue());
    SkScalar y = SkDoubleToScalar(args[1]->NumberValue());

    ctx->canvas->translate(x, y);
  }

  return scope.Close(Undefined());
}

METHOD(Transform) {
  HandleScope scope;

  if (!args[0]->IsUndefined() &&
      !args[1]->IsUndefined() &&
      !args[2]->IsUndefined() &&
      !args[3]->IsUndefined() &&
      !args[4]->IsUndefined() &&
      !args[5]->IsUndefined())
  {
    SkScalar a = SkDoubleToScalar(args[0]->NumberValue());
    SkScalar b = SkDoubleToScalar(args[1]->NumberValue());
    SkScalar c = SkDoubleToScalar(args[2]->NumberValue());
    SkScalar d = SkDoubleToScalar(args[3]->NumberValue());
    SkScalar e = SkDoubleToScalar(args[4]->NumberValue());
    SkScalar f = SkDoubleToScalar(args[5]->NumberValue());

    SkMatrix44 m;

    m.set3x3(
      a, c, e,
      b, d, f,
      0, 0, 1
    );

    Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

    assert(ctx->canvas->concat(m));
  }

  return scope.Close(Undefined());
}

METHOD(ResetMatrix) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->canvas->resetMatrix();

  return scope.Close(Undefined());
}

METHOD(SetGlobalAlpha) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->globalAlpha = args[0]->NumberValue()*255;

  if (ctx->globalAlpha > 255) {
    ctx->globalAlpha = 255;
  } else if (ctx->globalAlpha < 0) {
    ctx->globalAlpha = 0;
  }

  return scope.Close(Undefined());
}

METHOD(GetGlobalAlpha) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  return scope.Close(Number::New(ctx->globalAlpha/255));
}

METHOD(SetGlobalCompositeOperation) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->globalCompositeOperation = (SkXfermode::Mode)args[0]->IntegerValue();

  return scope.Close(Undefined());
}

METHOD(GetGlobalCompositeOperation) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetImageSmoothingEnabled) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetImageSmoothingEnabled) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetStrokeStyle) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetStrokeStyle) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetFillStylePattern) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  // if (!Buffer::HasInstance(args[0])) {
  //   return ThrowException(Exception::Error(
  //               String::New("First argument needs to be a buffer")));
  // }

  // Local<Object> buffer_obj = args[0]->ToObject();
  // char *buffer_data = Buffer::Data(buffer_obj);

  // double w = args[1]->NumberValue();
  // double h = args[2]->NumberValue();
  // SkShader::TileMode repeatX = args[3]->BooleanValue() ?
  //                              SkShader::kRepeat_TileMode :
  //                              SkShader::kClamp_TileMode;

  // SkShader::TileMode repeatY = args[4]->BooleanValue()?
  //                              SkShader::kRepeat_TileMode :
  //                              SkShader::kClamp_TileMode;

  // SkBitmap src;

  //src.setConfig(SkBitmap::kARGB_8888_Config, w, h);
  //src.setPixels(buffer_data);

  //SkBitmapProcShader shader(src, repeatX, repeatY);
  //ctx->paint.setShader(&shader);

  return scope.Close(Undefined());
}

METHOD(SetFillStyle) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  // Clear off the old shader
  ctx->paint.setShader(NULL);

  U8CPU a = args[3]->NumberValue();
  U8CPU r = args[0]->NumberValue();
  U8CPU g = args[1]->NumberValue();
  U8CPU b = args[2]->NumberValue();

  ctx->paint.setColor(SkColorSetARGBInline(a,r,g,b));

  return scope.Close(Undefined());
}

METHOD(GetFillStyle) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetLinearGradientShader) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkPoint points[2] = {
    {
      SkDoubleToScalar(args[0]->NumberValue()),
      SkDoubleToScalar(args[1]->NumberValue())
    },
    {
      SkDoubleToScalar(args[2]->NumberValue()),
      SkDoubleToScalar(args[3]->NumberValue())
    }
  };

  SkColor* colors = NULL;
  SkScalar* offsets = NULL;


  if (args[4]->IsArray()) {
    Handle<Array> stops = Handle<Array>::Cast(args[4]);
    uint32_t stopCount = stops->Length();


    if (stopCount > 1) {

      colors = new SkColor[stopCount];
      offsets = new SkScalar[stopCount];

      Handle<Object> item;
      Handle<Array> color;

      for (uint32_t stop = 0; stop < stopCount; stop++) {
        item = stops->Get(stop)->ToObject();

        offsets[stop] = SkDoubleToScalar(
          item->Get(String::NewSymbol("offset"))->NumberValue()
        );

        color = Handle<Array>::Cast(item->Get(String::NewSymbol("color")));

        colors[stop] = SkColorSetARGB(color->Get(3)->Uint32Value() & 0xff,
                                      color->Get(0)->Uint32Value() & 0xff,
                                      color->Get(1)->Uint32Value() & 0xff,
                                      color->Get(2)->Uint32Value() & 0xff);
      }


      SkShader *gradientShader = SkGradientShader::CreateLinear(
        points,
        colors,
        offsets,
        stopCount,
        SkShader::kRepeat_TileMode
      );

      ctx->paint.setShader(gradientShader);

      delete[] colors;
      delete[] offsets;

      gradientShader->unref();
    }
  }

  return scope.Close(Undefined());
}

METHOD(SetRadialGradientShader) {
 HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkPoint start = {
    SkDoubleToScalar(args[0]->NumberValue()),
    SkDoubleToScalar(args[1]->NumberValue())
  };

  SkScalar startRadius = SkDoubleToScalar(args[2]->NumberValue());

  SkPoint end = {
    SkDoubleToScalar(args[3]->NumberValue()),
    SkDoubleToScalar(args[4]->NumberValue())
  };

  SkScalar endRadius = SkDoubleToScalar(args[5]->NumberValue());


  SkColor* colors = NULL;
  SkScalar* offsets = NULL;


  if (args[6]->IsArray()) {
    Handle<Array> stops = Handle<Array>::Cast(args[6]);
    uint32_t stopCount = stops->Length();

    if (stopCount > 1) {

      colors = new SkColor[stopCount];
      offsets = new SkScalar[stopCount];

      Handle<Object> item;
      Handle<Array> color;

      for (uint32_t stop = 0; stop < stopCount; stop++) {
        item = stops->Get(stop)->ToObject();

        offsets[stop] = SkDoubleToScalar(
          item->Get(String::NewSymbol("offset"))->NumberValue()
        );

        color = Handle<Array>::Cast(item->Get(String::NewSymbol("color")));

        colors[stop] = SkColorSetARGB(color->Get(3)->Uint32Value() & 0xff,
                                      color->Get(0)->Uint32Value() & 0xff,
                                      color->Get(1)->Uint32Value() & 0xff,
                                      color->Get(2)->Uint32Value() & 0xff);
      }

      SkShader *gradientShader = SkGradientShader::CreateTwoPointConical(
        start,
        startRadius,
        end,
        endRadius,
        colors,
        offsets,
        stopCount,
        SkShader::kClamp_TileMode
      );

      assert(gradientShader);

      ctx->paint.setShader(gradientShader);

      delete[] colors;
      delete[] offsets;

      gradientShader->unref();

    }
  }

  return scope.Close(Undefined());
}

METHOD(SetShadowOffsetX) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->shadowX = args[0]->NumberValue();

  return scope.Close(Undefined());
}

METHOD(GetShadowOffsetX) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  return scope.Close(Number::New(ctx->shadowX));
}

METHOD(SetShadowOffsetY) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->shadowY = args[0]->NumberValue();

  return scope.Close(Undefined());
}

METHOD(GetShadowOffsetY) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  return scope.Close(Number::New(ctx->shadowY));
}

METHOD(SetShadowBlur) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->shadowBlur = args[0]->NumberValue();

  return scope.Close(Undefined());
}

METHOD(GetShadowBlur) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  return scope.Close(Number::New(ctx->shadowBlur));
}

METHOD(SetShadowColor) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  // Clear off the old shader
  ctx->shadowPaint.setShader(NULL);

  U8CPU a = args[3]->NumberValue();
  U8CPU r = args[0]->NumberValue();
  U8CPU g = args[1]->NumberValue();
  U8CPU b = args[2]->NumberValue();

  ctx->shadowPaint.setColor(SkColorSetARGBInline(a,r,g,b));


  return scope.Close(Undefined());
}

METHOD(GetShadowColor) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(ClearRect) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  SkCanvas *canvas = ctx->canvas;

  double x = args[0]->NumberValue();
  double y = args[1]->NumberValue();
  double w = args[2]->NumberValue();
  double h = args[3]->NumberValue();

  canvas->save();
  SkPaint clearPaint;
  clearPaint.setColor(SkColorSetARGBInline(0, 0, 0, 0));
  clearPaint.setXfermodeMode(SkXfermode::kSrc_Mode);

  ctx->canvas->drawRectCoords(
    x,
    y,
    x+w,
    y+h,
    clearPaint
  );

  canvas->restore();

  return scope.Close(Undefined());
}

METHOD(FillRect) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  double x = args[0]->NumberValue();
  double y = args[1]->NumberValue();
  double w = args[2]->NumberValue();
  double h = args[3]->NumberValue();

  double bx = (x < 0) ? x : 0;
  double by = (y < 0) ? y : 0;

  double dw = ctx->canvas->getDevice()->width();
  double dh = ctx->canvas->getDevice()->height();

  double bw = w+x > dw ? w+x : dw;
  double bh = h+y > dh ? h+y : dh;

  SkRect bounds = {
    bx, by, bw, bh
  };

  SkPaint p;
  p.setXfermodeMode(ctx->globalCompositeOperation);
  p.setAlpha(ctx->globalAlpha);

  int count = ctx->canvas->saveLayer(&bounds, &p);


  if (SkColorGetA(ctx->shadowPaint.getColor()) &&
      (ctx->shadowX || ctx->shadowY || ctx->shadowBlur)
     )
  {

    // Draw a shadow if applicable
    ctx->shadowPaint.setMaskFilter(SkBlurMaskFilter::Create(
      ctx->shadowBlur,
      SkBlurMaskFilter::kSolid_BlurStyle
      // TODO: consider SkBlurMaskFilter::kHighQuality_BlurFlag
    ));

    double sx = x+ctx->shadowX;
    double sy = y+ctx->shadowY;

    ctx->canvas->drawRectCoords(
      sx,
      sy,
      sx+w,
      sy+h,
      ctx->shadowPaint
    );
  }

  ctx->canvas->drawRectCoords(
    x,
    y,
    x+w,
    y+h,
    ctx->paint
  );

  ctx->canvas->restoreToCount(count);

  return scope.Close(Undefined());
}

METHOD(StrokeRect) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  double x = args[0]->NumberValue();
  double y = args[1]->NumberValue();
  double w = args[2]->NumberValue();
  double h = args[3]->NumberValue();

  double bx = (x < 0) ? x : 0;
  double by = (y < 0) ? y : 0;

  double dw = ctx->canvas->getDevice()->width();
  double dh = ctx->canvas->getDevice()->height();

  double bw = w+x > dw ? w+x : dw;
  double bh = h+y > dh ? h+y : dh;

  SkRect bounds = {
    bx, by, bw, bh
  };


  SkPaint paint(ctx->paint);
  ctx->canvas->drawRect(bounds, paint);


  return scope.Close(Undefined());
}

METHOD(BeginPath) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->path.rewind();

  return scope.Close(Undefined());
}

METHOD(Fill) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  ctx->canvas->save();
    ctx->canvas->resetMatrix();
    SkPaint fillPaint(ctx->paint);
    ctx->paint.setStyle(SkPaint::kFill_Style);
    ctx->canvas->drawPath(ctx->path, ctx->paint);
  ctx->canvas->restore();

  return scope.Close(Undefined());
}

METHOD(Stroke) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  ctx->canvas->save();
    ctx->canvas->resetMatrix();
    SkPaint strokePaint(ctx->paint);
    strokePaint.setStyle(SkPaint::kStroke_Style);
    ctx->canvas->drawPath(ctx->path, strokePaint);
  ctx->canvas->restore();

  return scope.Close(Undefined());
}

METHOD(Clip) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  // TODO: do we need to close before clipping?
  ctx->path.close();
  ctx->canvas->clipPath(ctx->path, SkRegion::kReplace_Op, true);

  return scope.Close(Undefined());
}

METHOD(IsPointInPath) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(ClosePath) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->path.close();

  return scope.Close(Undefined());
}

METHOD(MoveTo) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkPath subpath;

  subpath.moveTo(
    SkDoubleToScalar(args[0]->NumberValue()),
    SkDoubleToScalar(args[1]->NumberValue())
  );

  SkMatrix44 currentTransform(ctx->canvas->getTotalMatrix());
  subpath.transform(currentTransform);

  ctx->path.addPath(subpath);

  return scope.Close(Undefined());
}

METHOD(LineTo) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkScalar x = SkDoubleToScalar(args[0]->NumberValue());
  SkScalar y = SkDoubleToScalar(args[1]->NumberValue());

  SkPoint pt;
  if (!ctx->path.getLastPt(&pt)) {
    ctx->path.moveTo(x, y);
  }

  SkMatrix44 currentTransform(ctx->canvas->getTotalMatrix());
  ctx->path.transform(currentTransform);
  ctx->path.lineTo(x, y);
  ctx->path.setLastPt(x, y);

  return scope.Close(Undefined());
}

METHOD(QuadraticCurveTo) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(BezierCurveTo) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(ArcTo) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkScalar x1 = SkDoubleToScalar(args[0]->NumberValue());
  SkScalar y1 = SkDoubleToScalar(args[1]->NumberValue());
  SkScalar x2 = SkDoubleToScalar(args[2]->NumberValue());
  SkScalar y2 = SkDoubleToScalar(args[3]->NumberValue());
  SkScalar r = SkDoubleToScalar(args[4]->NumberValue());

  ctx->path.arcTo(x1, y1, x2, y2, r);

  return scope.Close(Undefined());
}

METHOD(Rect) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkScalar x = SkDoubleToScalar(args[0]->NumberValue());
  SkScalar y = SkDoubleToScalar(args[1]->NumberValue());
  SkScalar w = SkDoubleToScalar(args[2]->NumberValue());
  SkScalar h = SkDoubleToScalar(args[3]->NumberValue());

  SkRect src = SkRect::MakeXYWH(x, y, w, h);
  SkPath subpath;
  subpath.addRect(src);
  SkMatrix44 currentTransform(ctx->canvas->getTotalMatrix());
  subpath.transform(currentTransform);
  ctx->path.addPath(subpath);

  return scope.Close(Undefined());
}

METHOD(Arc) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  SkScalar x = SkDoubleToScalar(args[0]->NumberValue());
  SkScalar y = SkDoubleToScalar(args[1]->NumberValue());
  SkScalar r = SkDoubleToScalar(args[2]->NumberValue());
  SkScalar sa = SkDoubleToScalar(args[3]->NumberValue());
  SkScalar ea = SkDoubleToScalar(args[4]->NumberValue());
  bool ccw = args[5]->BooleanValue();



  // TODO: probably could get the last point
  //       and compare it with x/y
  if (!ctx->path.isEmpty()) {
    ctx->path.lineTo(x, y);
  }

  if (!ccw) {
    SkRect rect = {
      x-r, y-r, x+r, y+r
    };

    if (sa > ea + TAU) {
      ea = fmodf(ea, TAU);
    }

    ctx->path.addArc(rect, DEGREES(sa), DEGREES(ea));
  } else if (sa != ea) {

    if (sa > ea + TAU) {
      ctx->path.addCircle(x, y, r, SkPath::kCCW_Direction);
    } else {
      SkRect rect = {
        x+r, y+r, x-r, y-r
      };
      ctx->path.addArc(rect, DEGREES(ea), DEGREES(sa));
    }
  }

  return scope.Close(Undefined());
}

METHOD(Ellipse) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(FillText) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(StrokeText) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(MeasureText) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetFont) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetFont) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetTextAlign) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetTextAlign) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetTextBaseline) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetTextBaseline) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(DrawImageBuffer) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  // if (!Buffer::HasInstance(args[0])) {
  //   return ThrowException(Exception::Error(
  //               String::New("First argument needs to be a buffer")));
  // }

  Local<Object> buffer_obj = args[0]->ToObject();
  char *buffer_data = Buffer::Data(buffer_obj);

  double sx = args[1]->NumberValue();
  double sy = args[2]->NumberValue();
  double sw = args[3]->NumberValue();
  double sh = args[4]->NumberValue();
  double dx = args[5]->NumberValue();
  double dy = args[6]->NumberValue();
  double dw = args[7]->NumberValue();
  double dh = args[8]->NumberValue();
  double w = args[9]->NumberValue();
  double h = args[10]->NumberValue();

  SkBitmap src;

  src.setConfig(SkBitmap::kARGB_8888_Config, w, h);
  src.setPixels(buffer_data);

  SkRect srcRect = { sx, sy, sx+sw, sy+sh };
  SkRect destRect = { dx, dy, dx+dw, dy+dh };

  SkRect bounds = {
    0, 0,
    ctx->canvas->getDevice()->width(),
    ctx->canvas->getDevice()->height()
  };

  SkPaint layerPaint;
  layerPaint.setXfermodeMode(ctx->globalCompositeOperation);
  layerPaint.setAlpha(ctx->globalAlpha);

  int count = ctx->canvas->saveLayer(&bounds, &layerPaint);
  ctx->canvas->drawBitmapRectToRect(src, &srcRect, destRect, NULL);
  ctx->canvas->restoreToCount(count);

  return scope.Close(Undefined());
}

METHOD(CreateImageData) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetImageData) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  SkDevice* device = ctx->canvas->getDevice();

  int32_t sx = args[0]->IntegerValue();
  int32_t sy = args[1]->IntegerValue();
  int32_t sw = args[2]->IntegerValue();
  int32_t sh = args[3]->IntegerValue();
  SkIRect srcRect = { sx, sy, sx+sw, sy+sh };

  SkIRect bounds;
  bounds.set(0, 0, device->width(), device->height());
  if (!bounds.intersect(srcRect)) {
      return scope.Close(Undefined());
  }

  SkBitmap bitmap;
  bitmap.setConfig(
    SkBitmap::kARGB_8888_Config,
    bounds.width(),
    bounds.height()
  );

  if (ctx->canvas->readPixels(&bitmap, bounds.fLeft, bounds.fTop)) {

    size_t size = bitmap.getSize();

    bitmap.lockPixels();
    Buffer *buffer = Buffer::New(size);
    memcpy(Buffer::Data(buffer), (const char *)bitmap.getPixels(), size);
    bitmap.unlockPixels();

    Local<v8::Object> globalObj = v8::Context::GetCurrent()->Global();
    Local<Function> bufferConstructor = v8::Local<v8::Function>::Cast(globalObj->Get(v8::String::New("Buffer")));
    Handle<Value> constructorArgs[3] = { buffer->handle_, v8::Integer::New(Buffer::Length(buffer)), v8::Integer::New(0) };
    Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);

    Handle<Object> obj = Object::New();
    obj->Set(String::NewSymbol("width"), Number::New(bounds.width()));
    obj->Set(String::NewSymbol("height"), Number::New(bounds.height()));
    obj->Set(String::NewSymbol("data"), actualBuffer);

    return scope.Close(obj);
  }

  return scope.Close(Undefined());
}

METHOD(PutImageData) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetLineWidth) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  return scope.Close(Number::New(ctx->paint.getStrokeWidth()));
}

METHOD(SetLineWidth) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());
  ctx->paint.setStrokeWidth(SkDoubleToScalar(args[0]->NumberValue()));

  return scope.Close(Undefined());
}

METHOD(SetLineCap) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  int c = args[0]->IntegerValue();
  ctx->paint.setStrokeCap((SkPaint::Cap)c);

  return scope.Close(Undefined());
}

METHOD(SetLineJoin) {
  HandleScope scope;

  Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());

  int j = args[0]->IntegerValue();
  ctx->paint.setStrokeJoin((SkPaint::Join)j);

  return scope.Close(Undefined());
}

METHOD(GetMiterLimit) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetMiterLimit) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetLineDash) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetLineDash) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(SetLineDashOffset) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

METHOD(GetLineDashOffset) {
  HandleScope scope;

  // Context2D *ctx = ObjectWrap::Unwrap<Context2D>(args.This());



  return scope.Close(Undefined());
}

