#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'STL Reader'
cylinder1stl = STLReader(FileNames=['/home/sohnishi/workspace/gxsview/tests/auto/formula/logical/createCsgModel/cylinder1.stl'])

# create a new 'STL Reader'
cylinder1rstl = STLReader(FileNames=['/home/sohnishi/workspace/gxsview/tests/auto/formula/logical/createCsgModel/cylinder1r.stl'])

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1182, 913]

# get color transfer function/color map for 'STLSolidLabeling'
sTLSolidLabelingLUT = GetColorTransferFunction('STLSolidLabeling')
sTLSolidLabelingLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 5.878906683738906e-39, 0.865003, 0.865003, 0.865003, 1.1757813367477812e-38, 0.705882, 0.0156863, 0.14902]
sTLSolidLabelingLUT.ScalarRangeInitialized = 1.0

# show data in view
cylinder1stlDisplay = Show(cylinder1stl, renderView1)
# trace defaults for the display properties.
cylinder1stlDisplay.Representation = 'Surface'
cylinder1stlDisplay.ColorArrayName = ['CELLS', 'STLSolidLabeling']
cylinder1stlDisplay.LookupTable = sTLSolidLabelingLUT
cylinder1stlDisplay.OSPRayScaleArray = 'STLSolidLabeling'
cylinder1stlDisplay.OSPRayScaleFunction = 'PiecewiseFunction'
cylinder1stlDisplay.SelectOrientationVectors = 'None'
cylinder1stlDisplay.ScaleFactor = 8.480749893188477
cylinder1stlDisplay.SelectScaleArray = 'STLSolidLabeling'
cylinder1stlDisplay.GlyphType = 'Arrow'
cylinder1stlDisplay.GlyphTableIndexArray = 'STLSolidLabeling'
cylinder1stlDisplay.DataAxesGrid = 'GridAxesRepresentation'
cylinder1stlDisplay.PolarAxes = 'PolarAxesRepresentation'
cylinder1stlDisplay.GaussianRadius = 4.240374946594239
cylinder1stlDisplay.SetScaleArray = [None, '']
cylinder1stlDisplay.ScaleTransferFunction = 'PiecewiseFunction'
cylinder1stlDisplay.OpacityArray = [None, '']
cylinder1stlDisplay.OpacityTransferFunction = 'PiecewiseFunction'

# reset view to fit data
renderView1.ResetCamera()

# show color bar/color legend
cylinder1stlDisplay.SetScalarBarVisibility(renderView1, True)

# show data in view
cylinder1rstlDisplay = Show(cylinder1rstl, renderView1)
# trace defaults for the display properties.
cylinder1rstlDisplay.Representation = 'Surface'
cylinder1rstlDisplay.ColorArrayName = ['CELLS', 'STLSolidLabeling']
cylinder1rstlDisplay.LookupTable = sTLSolidLabelingLUT
cylinder1rstlDisplay.OSPRayScaleArray = 'STLSolidLabeling'
cylinder1rstlDisplay.OSPRayScaleFunction = 'PiecewiseFunction'
cylinder1rstlDisplay.SelectOrientationVectors = 'None'
cylinder1rstlDisplay.ScaleFactor = 8.480749893188477
cylinder1rstlDisplay.SelectScaleArray = 'STLSolidLabeling'
cylinder1rstlDisplay.GlyphType = 'Arrow'
cylinder1rstlDisplay.GlyphTableIndexArray = 'STLSolidLabeling'
cylinder1rstlDisplay.DataAxesGrid = 'GridAxesRepresentation'
cylinder1rstlDisplay.PolarAxes = 'PolarAxesRepresentation'
cylinder1rstlDisplay.GaussianRadius = 4.240374946594239
cylinder1rstlDisplay.SetScaleArray = [None, '']
cylinder1rstlDisplay.ScaleTransferFunction = 'PiecewiseFunction'
cylinder1rstlDisplay.OpacityArray = [None, '']
cylinder1rstlDisplay.OpacityTransferFunction = 'PiecewiseFunction'

# show color bar/color legend
cylinder1rstlDisplay.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# set active source
SetActiveSource(cylinder1stl)

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
cylinder1stlDisplay.DiffuseColor = [1.0, 0.0, 0.0]

# set active source
SetActiveSource(cylinder1rstl)

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
cylinder1rstlDisplay.DiffuseColor = [0.0, 0.0, 1.0]

# set active source
SetActiveSource(cylinder1stl)

# create a new 'Normal Glyphs'
normalGlyphs1 = NormalGlyphs(Input=cylinder1stl)

# Properties modified on normalGlyphs1
normalGlyphs1.GlyphScaleFactor = 10.0

# show data in view
normalGlyphs1Display = Show(normalGlyphs1, renderView1)
# trace defaults for the display properties.
normalGlyphs1Display.Representation = 'Surface'
normalGlyphs1Display.ColorArrayName = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.LookupTable = sTLSolidLabelingLUT
normalGlyphs1Display.OSPRayScaleArray = 'STLSolidLabeling'
normalGlyphs1Display.OSPRayScaleFunction = 'PiecewiseFunction'
normalGlyphs1Display.SelectOrientationVectors = 'GlyphVector'
normalGlyphs1Display.ScaleFactor = 3.907039833068848
normalGlyphs1Display.SelectScaleArray = 'STLSolidLabeling'
normalGlyphs1Display.GlyphType = 'Arrow'
normalGlyphs1Display.GlyphTableIndexArray = 'STLSolidLabeling'
normalGlyphs1Display.DataAxesGrid = 'GridAxesRepresentation'
normalGlyphs1Display.PolarAxes = 'PolarAxesRepresentation'
normalGlyphs1Display.GaussianRadius = 1.953519916534424
normalGlyphs1Display.SetScaleArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.ScaleTransferFunction = 'PiecewiseFunction'
normalGlyphs1Display.OpacityArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.OpacityTransferFunction = 'PiecewiseFunction'

# show color bar/color legend
normalGlyphs1Display.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
normalGlyphs1Display.DiffuseColor = [0.3333333333333333, 1.0, 1.0]

# set active source
SetActiveSource(cylinder1rstl)

# create a new 'Normal Glyphs'
normalGlyphs2 = NormalGlyphs(Input=cylinder1rstl)

# Properties modified on normalGlyphs2
normalGlyphs2.GlyphScaleFactor = 10.0

# show data in view
normalGlyphs2Display = Show(normalGlyphs2, renderView1)
# trace defaults for the display properties.
normalGlyphs2Display.Representation = 'Surface'
normalGlyphs2Display.ColorArrayName = ['POINTS', 'STLSolidLabeling']
normalGlyphs2Display.LookupTable = sTLSolidLabelingLUT
normalGlyphs2Display.OSPRayScaleArray = 'STLSolidLabeling'
normalGlyphs2Display.OSPRayScaleFunction = 'PiecewiseFunction'
normalGlyphs2Display.SelectOrientationVectors = 'GlyphVector'
normalGlyphs2Display.ScaleFactor = 3.907039833068848
normalGlyphs2Display.SelectScaleArray = 'STLSolidLabeling'
normalGlyphs2Display.GlyphType = 'Arrow'
normalGlyphs2Display.GlyphTableIndexArray = 'STLSolidLabeling'
normalGlyphs2Display.DataAxesGrid = 'GridAxesRepresentation'
normalGlyphs2Display.PolarAxes = 'PolarAxesRepresentation'
normalGlyphs2Display.GaussianRadius = 1.953519916534424
normalGlyphs2Display.SetScaleArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs2Display.ScaleTransferFunction = 'PiecewiseFunction'
normalGlyphs2Display.OpacityArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs2Display.OpacityTransferFunction = 'PiecewiseFunction'

# show color bar/color legend
normalGlyphs2Display.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
normalGlyphs2Display.DiffuseColor = [1.0, 0.3333333333333333, 1.0]

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [6.028750419616699, 6.198199272155762, 211.28633232581421]
renderView1.CameraFocalPoint = [6.028750419616699, 6.198199272155762, -0.8318500518798828]
renderView1.CameraParallelScale = 54.90022541187713

#### uncomment the following to render all views
#RenderAllViews()
#val = input()
# alternatively, if you want to write images, you can use SaveScreenshot(...).
