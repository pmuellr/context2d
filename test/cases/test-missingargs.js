var helpers = require('../helpers');
var test = helpers.test;
var Canvas = helpers.Canvas;
var Image = helpers.Image;
var Window = helpers.Window;
var Document = helpers.Document;
var DOMException = helpers.DOMException;
var wrapFunction = function(t, cb) { return function() { cb(); t.end() } };
test('2d.missingargs', function(t) {

  var canvas = new Canvas(100, 50);
  var ctx = canvas.getContext('2d')

  try { var _thrown = false;
    ctx.scale();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.scale()"); }
  try { var _thrown = false;
    ctx.scale(1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.scale(1)"); }
  try { var _thrown = false;
    ctx.rotate();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.rotate()"); }
  try { var _thrown = false;
    ctx.translate();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.translate()"); }
  try { var _thrown = false;
    ctx.translate(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.translate(0)"); }
  if (ctx.transform) { // (avoid spurious failures, since the aim here is not to test that all features are supported)
      try { var _thrown = false;
    ctx.transform();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform()"); }
      try { var _thrown = false;
    ctx.transform(1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform(1)"); }
      try { var _thrown = false;
    ctx.transform(1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform(1, 0)"); }
      try { var _thrown = false;
    ctx.transform(1, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform(1, 0, 0)"); }
      try { var _thrown = false;
    ctx.transform(1, 0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform(1, 0, 0, 1)"); }
      try { var _thrown = false;
    ctx.transform(1, 0, 0, 1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.transform(1, 0, 0, 1, 0)"); }
  }
  if (ctx.setTransform) {
      try { var _thrown = false;
    ctx.setTransform();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform()"); }
      try { var _thrown = false;
    ctx.setTransform(1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform(1)"); }
      try { var _thrown = false;
    ctx.setTransform(1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform(1, 0)"); }
      try { var _thrown = false;
    ctx.setTransform(1, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform(1, 0, 0)"); }
      try { var _thrown = false;
    ctx.setTransform(1, 0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform(1, 0, 0, 1)"); }
      try { var _thrown = false;
    ctx.setTransform(1, 0, 0, 1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.setTransform(1, 0, 0, 1, 0)"); }
  }
  try { var _thrown = false;
    ctx.createLinearGradient();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createLinearGradient()"); }
  try { var _thrown = false;
    ctx.createLinearGradient(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createLinearGradient(0)"); }
  try { var _thrown = false;
    ctx.createLinearGradient(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createLinearGradient(0, 0)"); }
  try { var _thrown = false;
    ctx.createLinearGradient(0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createLinearGradient(0, 0, 1)"); }
  try { var _thrown = false;
    ctx.createRadialGradient();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient()"); }
  try { var _thrown = false;
    ctx.createRadialGradient(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient(0)"); }
  try { var _thrown = false;
    ctx.createRadialGradient(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient(0, 0)"); }
  try { var _thrown = false;
    ctx.createRadialGradient(0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient(0, 0, 1)"); }
  try { var _thrown = false;
    ctx.createRadialGradient(0, 0, 1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient(0, 0, 1, 0)"); }
  try { var _thrown = false;
    ctx.createRadialGradient(0, 0, 1, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createRadialGradient(0, 0, 1, 0, 0)"); }
  try { var _thrown = false;
    ctx.createPattern(canvas);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createPattern(canvas)"); }
  try { var _thrown = false;
    ctx.clearRect();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.clearRect()"); }
  try { var _thrown = false;
    ctx.clearRect(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.clearRect(0)"); }
  try { var _thrown = false;
    ctx.clearRect(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.clearRect(0, 0)"); }
  try { var _thrown = false;
    ctx.clearRect(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.clearRect(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.fillRect();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillRect()"); }
  try { var _thrown = false;
    ctx.fillRect(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillRect(0)"); }
  try { var _thrown = false;
    ctx.fillRect(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillRect(0, 0)"); }
  try { var _thrown = false;
    ctx.fillRect(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillRect(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.strokeRect();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeRect()"); }
  try { var _thrown = false;
    ctx.strokeRect(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeRect(0)"); }
  try { var _thrown = false;
    ctx.strokeRect(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeRect(0, 0)"); }
  try { var _thrown = false;
    ctx.strokeRect(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeRect(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.moveTo();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.moveTo()"); }
  try { var _thrown = false;
    ctx.moveTo(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.moveTo(0)"); }
  try { var _thrown = false;
    ctx.lineTo();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.lineTo()"); }
  try { var _thrown = false;
    ctx.lineTo(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.lineTo(0)"); }
  try { var _thrown = false;
    ctx.quadraticCurveTo();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.quadraticCurveTo()"); }
  try { var _thrown = false;
    ctx.quadraticCurveTo(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.quadraticCurveTo(0)"); }
  try { var _thrown = false;
    ctx.quadraticCurveTo(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.quadraticCurveTo(0, 0)"); }
  try { var _thrown = false;
    ctx.quadraticCurveTo(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.quadraticCurveTo(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.bezierCurveTo();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo()"); }
  try { var _thrown = false;
    ctx.bezierCurveTo(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo(0)"); }
  try { var _thrown = false;
    ctx.bezierCurveTo(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo(0, 0)"); }
  try { var _thrown = false;
    ctx.bezierCurveTo(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.bezierCurveTo(0, 0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo(0, 0, 0, 0)"); }
  try { var _thrown = false;
    ctx.bezierCurveTo(0, 0, 0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.bezierCurveTo(0, 0, 0, 0, 0)"); }
  try { var _thrown = false;
    ctx.arcTo();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arcTo()"); }
  try { var _thrown = false;
    ctx.arcTo(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arcTo(0)"); }
  try { var _thrown = false;
    ctx.arcTo(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arcTo(0, 0)"); }
  try { var _thrown = false;
    ctx.arcTo(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arcTo(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.arcTo(0, 0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arcTo(0, 0, 0, 0)"); }
  try { var _thrown = false;
    ctx.rect();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.rect()"); }
  try { var _thrown = false;
    ctx.rect(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.rect(0)"); }
  try { var _thrown = false;
    ctx.rect(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.rect(0, 0)"); }
  try { var _thrown = false;
    ctx.rect(0, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.rect(0, 0, 0)"); }
  try { var _thrown = false;
    ctx.arc();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc()"); }
  try { var _thrown = false;
    ctx.arc(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc(0)"); }
  try { var _thrown = false;
    ctx.arc(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc(0, 0)"); }
  try { var _thrown = false;
    ctx.arc(0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc(0, 0, 1)"); }
  try { var _thrown = false;
    ctx.arc(0, 0, 1, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc(0, 0, 1, 0)"); }
  try { var _thrown = false;
    ctx.arc(0, 0, 1, 0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.arc(0, 0, 1, 0, 0)"); }
  if (ctx.isPointInPath) {
      try { var _thrown = false;
    ctx.isPointInPath();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.isPointInPath()"); }
      try { var _thrown = false;
    ctx.isPointInPath(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.isPointInPath(0)"); }
  }
  if (ctx.drawFocusRing) {
      try { var _thrown = false;
    ctx.drawFocusRing();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawFocusRing()"); }
      try { var _thrown = false;
    ctx.drawFocusRing(canvas);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawFocusRing(canvas)"); }
      try { var _thrown = false;
    ctx.drawFocusRing(canvas, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawFocusRing(canvas, 0)"); }
  }
  if (ctx.fillText) {
      try { var _thrown = false;
    ctx.fillText();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillText()"); }
      try { var _thrown = false;
    ctx.fillText('test');
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillText('test')"); }
      try { var _thrown = false;
    ctx.fillText('test', 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.fillText('test', 0)"); }
      try { var _thrown = false;
    ctx.strokeText();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeText()"); }
      try { var _thrown = false;
    ctx.strokeText('test');
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeText('test')"); }
      try { var _thrown = false;
    ctx.strokeText('test', 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.strokeText('test', 0)"); }
      try { var _thrown = false;
    ctx.measureText();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.measureText()"); }
  }
  try { var _thrown = false;
    ctx.drawImage();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawImage()"); }
  try { var _thrown = false;
    ctx.drawImage(canvas);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawImage(canvas)"); }
  try { var _thrown = false;
    ctx.drawImage(canvas, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.drawImage(canvas, 0)"); }
  // TODO: n >= 3 args on drawImage could be either a valid overload,
  // or too few for another overload, or too many for another
  // overload - what should happen?
  if (ctx.createImageData) {
      try { var _thrown = false;
    ctx.createImageData();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createImageData()"); }
      try { var _thrown = false;
    ctx.createImageData(1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.createImageData(1)"); }
  }
  if (ctx.getImageData) {
      try { var _thrown = false;
    ctx.getImageData();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.getImageData()"); }
      try { var _thrown = false;
    ctx.getImageData(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.getImageData(0)"); }
      try { var _thrown = false;
    ctx.getImageData(0, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.getImageData(0, 0)"); }
      try { var _thrown = false;
    ctx.getImageData(0, 0, 1);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.getImageData(0, 0, 1)"); }
  }
  if (ctx.putImageData) {
      var imgdata = ctx.getImageData(0, 0, 1, 1);
      try { var _thrown = false;
    ctx.putImageData();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.putImageData()"); }
      try { var _thrown = false;
    ctx.putImageData(imgdata);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.putImageData(imgdata)"); }
      try { var _thrown = false;
    ctx.putImageData(imgdata, 0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: ctx.putImageData(imgdata, 0)"); }
  }
  var g = ctx.createLinearGradient(0, 0, 0, 0);
  try { var _thrown = false;
    g.addColorStop();
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: g.addColorStop()"); }
  try { var _thrown = false;
    g.addColorStop(0);
  } catch (e) { if (e.code != DOMException.NOT_SUPPORTED_ERR) t.fail("Failed assertion: expected exception of type NOT_SUPPORTED_ERR, got: "+e.message); _thrown = true; } finally { helpers.ok(t, _thrown, "should throw exception of type NOT_SUPPORTED_ERR: g.addColorStop(0)"); }

  t.end()
});

