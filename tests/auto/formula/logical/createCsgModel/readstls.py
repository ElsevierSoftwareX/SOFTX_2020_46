#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# create a new 'STL Reader'
cylinder1stl = STLReader(FileNames=['/home/sohnishi/workspace/gxsview/tests/auto/formula/logical/createCsgModel/cylinder1.stl'])

# get active view
renderView1 = GetActiveViewOrCreate('RenderView')
# uncomment following to set a specific view size
# renderView1.ViewSize = [1182, 916]

# get color transfer function/color map for 'STLSolidLabeling'
sTLSolidLabelingLUT = GetColorTransferFunction('STLSolidLabeling')
sTLSolidLabelingLUT.LockDataRange = 0
sTLSolidLabelingLUT.InterpretValuesAsCategories = 0
sTLSolidLabelingLUT.ShowCategoricalColorsinDataRangeOnly = 0
sTLSolidLabelingLUT.RescaleOnVisibilityChange = 0
sTLSolidLabelingLUT.EnableOpacityMapping = 0
sTLSolidLabelingLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 5.878906683738906e-39, 0.865003, 0.865003, 0.865003, 1.1757813367477812e-38, 0.705882, 0.0156863, 0.14902]
sTLSolidLabelingLUT.UseLogScale = 0
sTLSolidLabelingLUT.ColorSpace = 'Diverging'
sTLSolidLabelingLUT.UseBelowRangeColor = 0
sTLSolidLabelingLUT.BelowRangeColor = [0.0, 0.0, 0.0]
sTLSolidLabelingLUT.UseAboveRangeColor = 0
sTLSolidLabelingLUT.AboveRangeColor = [1.0, 1.0, 1.0]
sTLSolidLabelingLUT.NanColor = [1.0, 1.0, 0.0]
sTLSolidLabelingLUT.Discretize = 1
sTLSolidLabelingLUT.NumberOfTableValues = 256
sTLSolidLabelingLUT.ScalarRangeInitialized = 1.0
sTLSolidLabelingLUT.HSVWrap = 0
sTLSolidLabelingLUT.VectorComponent = 0
sTLSolidLabelingLUT.VectorMode = 'Magnitude'
sTLSolidLabelingLUT.AllowDuplicateScalars = 1
sTLSolidLabelingLUT.Annotations = []
sTLSolidLabelingLUT.ActiveAnnotatedValues = []
sTLSolidLabelingLUT.IndexedColors = []

# show data in view
cylinder1stlDisplay = Show(cylinder1stl, renderView1)
# trace defaults for the display properties.
cylinder1stlDisplay.Representation = 'Surface'
cylinder1stlDisplay.AmbientColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.ColorArrayName = ['CELLS', 'STLSolidLabeling']
cylinder1stlDisplay.DiffuseColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.LookupTable = sTLSolidLabelingLUT
cylinder1stlDisplay.MapScalars = 1
cylinder1stlDisplay.InterpolateScalarsBeforeMapping = 1
cylinder1stlDisplay.Opacity = 1.0
cylinder1stlDisplay.PointSize = 2.0
cylinder1stlDisplay.LineWidth = 1.0
cylinder1stlDisplay.Interpolation = 'Gouraud'
cylinder1stlDisplay.Specular = 0.0
cylinder1stlDisplay.SpecularColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.SpecularPower = 100.0
cylinder1stlDisplay.Ambient = 0.0
cylinder1stlDisplay.Diffuse = 1.0
cylinder1stlDisplay.EdgeColor = [0.0, 0.0, 0.5]
cylinder1stlDisplay.BackfaceRepresentation = 'Follow Frontface'
cylinder1stlDisplay.BackfaceAmbientColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.BackfaceDiffuseColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.BackfaceOpacity = 1.0
cylinder1stlDisplay.Position = [0.0, 0.0, 0.0]
cylinder1stlDisplay.Scale = [1.0, 1.0, 1.0]
cylinder1stlDisplay.Orientation = [0.0, 0.0, 0.0]
cylinder1stlDisplay.Origin = [0.0, 0.0, 0.0]
cylinder1stlDisplay.Pickable = 1
cylinder1stlDisplay.Texture = None
cylinder1stlDisplay.Triangulate = 0
cylinder1stlDisplay.NonlinearSubdivisionLevel = 1
cylinder1stlDisplay.UseDataPartitions = 0
cylinder1stlDisplay.OSPRayUseScaleArray = 0
cylinder1stlDisplay.OSPRayScaleArray = 'STLSolidLabeling'
cylinder1stlDisplay.OSPRayScaleFunction = 'PiecewiseFunction'
cylinder1stlDisplay.Orient = 0
cylinder1stlDisplay.OrientationMode = 'Direction'
cylinder1stlDisplay.SelectOrientationVectors = 'None'
cylinder1stlDisplay.Scaling = 0
cylinder1stlDisplay.ScaleMode = 'No Data Scaling Off'
cylinder1stlDisplay.ScaleFactor = 8.480749893188477
cylinder1stlDisplay.SelectScaleArray = 'STLSolidLabeling'
cylinder1stlDisplay.GlyphType = 'Arrow'
cylinder1stlDisplay.UseGlyphTable = 0
cylinder1stlDisplay.GlyphTableIndexArray = 'STLSolidLabeling'
cylinder1stlDisplay.UseCompositeGlyphTable = 0
cylinder1stlDisplay.DataAxesGrid = 'GridAxesRepresentation'
cylinder1stlDisplay.SelectionCellLabelBold = 0
cylinder1stlDisplay.SelectionCellLabelColor = [0.0, 1.0, 0.0]
cylinder1stlDisplay.SelectionCellLabelFontFamily = 'Arial'
cylinder1stlDisplay.SelectionCellLabelFontSize = 18
cylinder1stlDisplay.SelectionCellLabelItalic = 0
cylinder1stlDisplay.SelectionCellLabelJustification = 'Left'
cylinder1stlDisplay.SelectionCellLabelOpacity = 1.0
cylinder1stlDisplay.SelectionCellLabelShadow = 0
cylinder1stlDisplay.SelectionPointLabelBold = 0
cylinder1stlDisplay.SelectionPointLabelColor = [1.0, 1.0, 0.0]
cylinder1stlDisplay.SelectionPointLabelFontFamily = 'Arial'
cylinder1stlDisplay.SelectionPointLabelFontSize = 18
cylinder1stlDisplay.SelectionPointLabelItalic = 0
cylinder1stlDisplay.SelectionPointLabelJustification = 'Left'
cylinder1stlDisplay.SelectionPointLabelOpacity = 1.0
cylinder1stlDisplay.SelectionPointLabelShadow = 0
cylinder1stlDisplay.PolarAxes = 'PolarAxesRepresentation'
cylinder1stlDisplay.GaussianRadius = 4.240374946594239
cylinder1stlDisplay.ShaderPreset = 'Sphere'
cylinder1stlDisplay.Emissive = 0
cylinder1stlDisplay.ScaleByArray = 0
cylinder1stlDisplay.SetScaleArray = [None, '']
cylinder1stlDisplay.ScaleTransferFunction = 'PiecewiseFunction'
cylinder1stlDisplay.OpacityByArray = 0
cylinder1stlDisplay.OpacityArray = [None, '']
cylinder1stlDisplay.OpacityTransferFunction = 'PiecewiseFunction'

# init the 'PiecewiseFunction' selected for 'OSPRayScaleFunction'
cylinder1stlDisplay.OSPRayScaleFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'Arrow' selected for 'GlyphType'
cylinder1stlDisplay.GlyphType.TipResolution = 6
cylinder1stlDisplay.GlyphType.TipRadius = 0.1
cylinder1stlDisplay.GlyphType.TipLength = 0.35
cylinder1stlDisplay.GlyphType.ShaftResolution = 6
cylinder1stlDisplay.GlyphType.ShaftRadius = 0.03
cylinder1stlDisplay.GlyphType.Invert = 0

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
cylinder1stlDisplay.DataAxesGrid.XTitle = 'X Axis'
cylinder1stlDisplay.DataAxesGrid.YTitle = 'Y Axis'
cylinder1stlDisplay.DataAxesGrid.ZTitle = 'Z Axis'
cylinder1stlDisplay.DataAxesGrid.XTitleColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.XTitleFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.XTitleBold = 0
cylinder1stlDisplay.DataAxesGrid.XTitleItalic = 0
cylinder1stlDisplay.DataAxesGrid.XTitleFontSize = 12
cylinder1stlDisplay.DataAxesGrid.XTitleShadow = 0
cylinder1stlDisplay.DataAxesGrid.XTitleOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.YTitleColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.YTitleFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.YTitleBold = 0
cylinder1stlDisplay.DataAxesGrid.YTitleItalic = 0
cylinder1stlDisplay.DataAxesGrid.YTitleFontSize = 12
cylinder1stlDisplay.DataAxesGrid.YTitleShadow = 0
cylinder1stlDisplay.DataAxesGrid.YTitleOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.ZTitleColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.ZTitleFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.ZTitleBold = 0
cylinder1stlDisplay.DataAxesGrid.ZTitleItalic = 0
cylinder1stlDisplay.DataAxesGrid.ZTitleFontSize = 12
cylinder1stlDisplay.DataAxesGrid.ZTitleShadow = 0
cylinder1stlDisplay.DataAxesGrid.ZTitleOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.FacesToRender = 63
cylinder1stlDisplay.DataAxesGrid.CullBackface = 0
cylinder1stlDisplay.DataAxesGrid.CullFrontface = 1
cylinder1stlDisplay.DataAxesGrid.GridColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.ShowGrid = 0
cylinder1stlDisplay.DataAxesGrid.ShowEdges = 1
cylinder1stlDisplay.DataAxesGrid.ShowTicks = 1
cylinder1stlDisplay.DataAxesGrid.LabelUniqueEdgesOnly = 1
cylinder1stlDisplay.DataAxesGrid.AxesToLabel = 63
cylinder1stlDisplay.DataAxesGrid.XLabelColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.XLabelFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.XLabelBold = 0
cylinder1stlDisplay.DataAxesGrid.XLabelItalic = 0
cylinder1stlDisplay.DataAxesGrid.XLabelFontSize = 12
cylinder1stlDisplay.DataAxesGrid.XLabelShadow = 0
cylinder1stlDisplay.DataAxesGrid.XLabelOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.YLabelColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.YLabelFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.YLabelBold = 0
cylinder1stlDisplay.DataAxesGrid.YLabelItalic = 0
cylinder1stlDisplay.DataAxesGrid.YLabelFontSize = 12
cylinder1stlDisplay.DataAxesGrid.YLabelShadow = 0
cylinder1stlDisplay.DataAxesGrid.YLabelOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.ZLabelColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.DataAxesGrid.ZLabelFontFamily = 'Arial'
cylinder1stlDisplay.DataAxesGrid.ZLabelBold = 0
cylinder1stlDisplay.DataAxesGrid.ZLabelItalic = 0
cylinder1stlDisplay.DataAxesGrid.ZLabelFontSize = 12
cylinder1stlDisplay.DataAxesGrid.ZLabelShadow = 0
cylinder1stlDisplay.DataAxesGrid.ZLabelOpacity = 1.0
cylinder1stlDisplay.DataAxesGrid.XAxisNotation = 'Mixed'
cylinder1stlDisplay.DataAxesGrid.XAxisPrecision = 2
cylinder1stlDisplay.DataAxesGrid.XAxisUseCustomLabels = 0
cylinder1stlDisplay.DataAxesGrid.XAxisLabels = []
cylinder1stlDisplay.DataAxesGrid.YAxisNotation = 'Mixed'
cylinder1stlDisplay.DataAxesGrid.YAxisPrecision = 2
cylinder1stlDisplay.DataAxesGrid.YAxisUseCustomLabels = 0
cylinder1stlDisplay.DataAxesGrid.YAxisLabels = []
cylinder1stlDisplay.DataAxesGrid.ZAxisNotation = 'Mixed'
cylinder1stlDisplay.DataAxesGrid.ZAxisPrecision = 2
cylinder1stlDisplay.DataAxesGrid.ZAxisUseCustomLabels = 0
cylinder1stlDisplay.DataAxesGrid.ZAxisLabels = []

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
cylinder1stlDisplay.PolarAxes.Visibility = 0
cylinder1stlDisplay.PolarAxes.Translation = [0.0, 0.0, 0.0]
cylinder1stlDisplay.PolarAxes.Scale = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.Orientation = [0.0, 0.0, 0.0]
cylinder1stlDisplay.PolarAxes.EnableCustomBounds = [0, 0, 0]
cylinder1stlDisplay.PolarAxes.CustomBounds = [0.0, 1.0, 0.0, 1.0, 0.0, 1.0]
cylinder1stlDisplay.PolarAxes.EnableCustomRange = 0
cylinder1stlDisplay.PolarAxes.CustomRange = [0.0, 1.0]
cylinder1stlDisplay.PolarAxes.PolarAxisVisibility = 1
cylinder1stlDisplay.PolarAxes.RadialAxesVisibility = 1
cylinder1stlDisplay.PolarAxes.DrawRadialGridlines = 1
cylinder1stlDisplay.PolarAxes.PolarArcsVisibility = 1
cylinder1stlDisplay.PolarAxes.DrawPolarArcsGridlines = 1
cylinder1stlDisplay.PolarAxes.NumberOfRadialAxes = 0
cylinder1stlDisplay.PolarAxes.AutoSubdividePolarAxis = 1
cylinder1stlDisplay.PolarAxes.NumberOfPolarAxis = 0
cylinder1stlDisplay.PolarAxes.MinimumRadius = 0.0
cylinder1stlDisplay.PolarAxes.MinimumAngle = 0.0
cylinder1stlDisplay.PolarAxes.MaximumAngle = 90.0
cylinder1stlDisplay.PolarAxes.RadialAxesOriginToPolarAxis = 1
cylinder1stlDisplay.PolarAxes.Ratio = 1.0
cylinder1stlDisplay.PolarAxes.PolarAxisColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.PolarArcsColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.LastRadialAxisColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.SecondaryPolarArcsColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.PolarAxisTitleVisibility = 1
cylinder1stlDisplay.PolarAxes.PolarAxisTitle = 'Radial Distance'
cylinder1stlDisplay.PolarAxes.PolarAxisTitleLocation = 'Bottom'
cylinder1stlDisplay.PolarAxes.PolarLabelVisibility = 1
cylinder1stlDisplay.PolarAxes.PolarLabelFormat = '%-#6.3g'
cylinder1stlDisplay.PolarAxes.PolarLabelExponentLocation = 'Labels'
cylinder1stlDisplay.PolarAxes.RadialLabelVisibility = 1
cylinder1stlDisplay.PolarAxes.RadialLabelFormat = '%-#3.1f'
cylinder1stlDisplay.PolarAxes.RadialLabelLocation = 'Bottom'
cylinder1stlDisplay.PolarAxes.RadialUnitsVisibility = 1
cylinder1stlDisplay.PolarAxes.ScreenSize = 10.0
cylinder1stlDisplay.PolarAxes.PolarAxisTitleColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.PolarAxisTitleOpacity = 1.0
cylinder1stlDisplay.PolarAxes.PolarAxisTitleFontFamily = 'Arial'
cylinder1stlDisplay.PolarAxes.PolarAxisTitleBold = 0
cylinder1stlDisplay.PolarAxes.PolarAxisTitleItalic = 0
cylinder1stlDisplay.PolarAxes.PolarAxisTitleShadow = 0
cylinder1stlDisplay.PolarAxes.PolarAxisTitleFontSize = 12
cylinder1stlDisplay.PolarAxes.PolarAxisLabelColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.PolarAxisLabelOpacity = 1.0
cylinder1stlDisplay.PolarAxes.PolarAxisLabelFontFamily = 'Arial'
cylinder1stlDisplay.PolarAxes.PolarAxisLabelBold = 0
cylinder1stlDisplay.PolarAxes.PolarAxisLabelItalic = 0
cylinder1stlDisplay.PolarAxes.PolarAxisLabelShadow = 0
cylinder1stlDisplay.PolarAxes.PolarAxisLabelFontSize = 12
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextOpacity = 1.0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextFontFamily = 'Arial'
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextBold = 0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextItalic = 0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextShadow = 0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTextFontSize = 12
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextColor = [1.0, 1.0, 1.0]
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextOpacity = 1.0
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextFontFamily = 'Arial'
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextBold = 0
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextItalic = 0
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextShadow = 0
cylinder1stlDisplay.PolarAxes.SecondaryRadialAxesTextFontSize = 12
cylinder1stlDisplay.PolarAxes.EnableDistanceLOD = 1
cylinder1stlDisplay.PolarAxes.DistanceLODThreshold = 0.7
cylinder1stlDisplay.PolarAxes.EnableViewAngleLOD = 1
cylinder1stlDisplay.PolarAxes.ViewAngleLODThreshold = 0.7
cylinder1stlDisplay.PolarAxes.SmallestVisiblePolarAngle = 0.5
cylinder1stlDisplay.PolarAxes.PolarTicksVisibility = 1
cylinder1stlDisplay.PolarAxes.ArcTicksOriginToPolarAxis = 1
cylinder1stlDisplay.PolarAxes.TickLocation = 'Both'
cylinder1stlDisplay.PolarAxes.AxisTickVisibility = 1
cylinder1stlDisplay.PolarAxes.AxisMinorTickVisibility = 0
cylinder1stlDisplay.PolarAxes.ArcTickVisibility = 1
cylinder1stlDisplay.PolarAxes.ArcMinorTickVisibility = 0
cylinder1stlDisplay.PolarAxes.DeltaAngleMajor = 10.0
cylinder1stlDisplay.PolarAxes.DeltaAngleMinor = 5.0
cylinder1stlDisplay.PolarAxes.PolarAxisMajorTickSize = 0.0
cylinder1stlDisplay.PolarAxes.PolarAxisTickRatioSize = 0.3
cylinder1stlDisplay.PolarAxes.PolarAxisMajorTickThickness = 1.0
cylinder1stlDisplay.PolarAxes.PolarAxisTickRatioThickness = 0.5
cylinder1stlDisplay.PolarAxes.LastRadialAxisMajorTickSize = 0.0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTickRatioSize = 0.3
cylinder1stlDisplay.PolarAxes.LastRadialAxisMajorTickThickness = 1.0
cylinder1stlDisplay.PolarAxes.LastRadialAxisTickRatioThickness = 0.5
cylinder1stlDisplay.PolarAxes.ArcMajorTickSize = 0.0
cylinder1stlDisplay.PolarAxes.ArcTickRatioSize = 0.3
cylinder1stlDisplay.PolarAxes.ArcMajorTickThickness = 1.0
cylinder1stlDisplay.PolarAxes.ArcTickRatioThickness = 0.5
cylinder1stlDisplay.PolarAxes.Use2DMode = 0
cylinder1stlDisplay.PolarAxes.UseLogAxis = 0

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
cylinder1stlDisplay.ScaleTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
cylinder1stlDisplay.OpacityTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# reset view to fit data
renderView1.ResetCamera()

# show color bar/color legend
cylinder1stlDisplay.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
cylinder1stlDisplay.DiffuseColor = [0.0, 0.3333333333333333, 1.0]

# change representation type
cylinder1stlDisplay.SetRepresentationType('Surface With Edges')

# create a new 'Normal Glyphs'
normalGlyphs1 = NormalGlyphs(Input=cylinder1stl)
normalGlyphs1.Consistency = 1
normalGlyphs1.GlyphMaxPoints = 5000
normalGlyphs1.GlyphScaleFactor = 1.0
normalGlyphs1.InvertArrow = 0

# Properties modified on normalGlyphs1
normalGlyphs1.GlyphScaleFactor = 10.0

# show data in view
normalGlyphs1Display = Show(normalGlyphs1, renderView1)
# trace defaults for the display properties.
normalGlyphs1Display.Representation = 'Surface'
normalGlyphs1Display.AmbientColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.ColorArrayName = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.DiffuseColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.LookupTable = sTLSolidLabelingLUT
normalGlyphs1Display.MapScalars = 1
normalGlyphs1Display.InterpolateScalarsBeforeMapping = 1
normalGlyphs1Display.Opacity = 1.0
normalGlyphs1Display.PointSize = 2.0
normalGlyphs1Display.LineWidth = 1.0
normalGlyphs1Display.Interpolation = 'Gouraud'
normalGlyphs1Display.Specular = 0.0
normalGlyphs1Display.SpecularColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.SpecularPower = 100.0
normalGlyphs1Display.Ambient = 0.0
normalGlyphs1Display.Diffuse = 1.0
normalGlyphs1Display.EdgeColor = [0.0, 0.0, 0.5]
normalGlyphs1Display.BackfaceRepresentation = 'Follow Frontface'
normalGlyphs1Display.BackfaceAmbientColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.BackfaceDiffuseColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.BackfaceOpacity = 1.0
normalGlyphs1Display.Position = [0.0, 0.0, 0.0]
normalGlyphs1Display.Scale = [1.0, 1.0, 1.0]
normalGlyphs1Display.Orientation = [0.0, 0.0, 0.0]
normalGlyphs1Display.Origin = [0.0, 0.0, 0.0]
normalGlyphs1Display.Pickable = 1
normalGlyphs1Display.Texture = None
normalGlyphs1Display.Triangulate = 0
normalGlyphs1Display.NonlinearSubdivisionLevel = 1
normalGlyphs1Display.UseDataPartitions = 0
normalGlyphs1Display.OSPRayUseScaleArray = 0
normalGlyphs1Display.OSPRayScaleArray = 'STLSolidLabeling'
normalGlyphs1Display.OSPRayScaleFunction = 'PiecewiseFunction'
normalGlyphs1Display.Orient = 0
normalGlyphs1Display.OrientationMode = 'Direction'
normalGlyphs1Display.SelectOrientationVectors = 'GlyphVector'
normalGlyphs1Display.Scaling = 0
normalGlyphs1Display.ScaleMode = 'No Data Scaling Off'
normalGlyphs1Display.ScaleFactor = 3.907039833068848
normalGlyphs1Display.SelectScaleArray = 'STLSolidLabeling'
normalGlyphs1Display.GlyphType = 'Arrow'
normalGlyphs1Display.UseGlyphTable = 0
normalGlyphs1Display.GlyphTableIndexArray = 'STLSolidLabeling'
normalGlyphs1Display.UseCompositeGlyphTable = 0
normalGlyphs1Display.DataAxesGrid = 'GridAxesRepresentation'
normalGlyphs1Display.SelectionCellLabelBold = 0
normalGlyphs1Display.SelectionCellLabelColor = [0.0, 1.0, 0.0]
normalGlyphs1Display.SelectionCellLabelFontFamily = 'Arial'
normalGlyphs1Display.SelectionCellLabelFontSize = 18
normalGlyphs1Display.SelectionCellLabelItalic = 0
normalGlyphs1Display.SelectionCellLabelJustification = 'Left'
normalGlyphs1Display.SelectionCellLabelOpacity = 1.0
normalGlyphs1Display.SelectionCellLabelShadow = 0
normalGlyphs1Display.SelectionPointLabelBold = 0
normalGlyphs1Display.SelectionPointLabelColor = [1.0, 1.0, 0.0]
normalGlyphs1Display.SelectionPointLabelFontFamily = 'Arial'
normalGlyphs1Display.SelectionPointLabelFontSize = 18
normalGlyphs1Display.SelectionPointLabelItalic = 0
normalGlyphs1Display.SelectionPointLabelJustification = 'Left'
normalGlyphs1Display.SelectionPointLabelOpacity = 1.0
normalGlyphs1Display.SelectionPointLabelShadow = 0
normalGlyphs1Display.PolarAxes = 'PolarAxesRepresentation'
normalGlyphs1Display.GaussianRadius = 1.953519916534424
normalGlyphs1Display.ShaderPreset = 'Sphere'
normalGlyphs1Display.Emissive = 0
normalGlyphs1Display.ScaleByArray = 0
normalGlyphs1Display.SetScaleArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.ScaleTransferFunction = 'PiecewiseFunction'
normalGlyphs1Display.OpacityByArray = 0
normalGlyphs1Display.OpacityArray = ['POINTS', 'STLSolidLabeling']
normalGlyphs1Display.OpacityTransferFunction = 'PiecewiseFunction'

# init the 'PiecewiseFunction' selected for 'OSPRayScaleFunction'
normalGlyphs1Display.OSPRayScaleFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'Arrow' selected for 'GlyphType'
normalGlyphs1Display.GlyphType.TipResolution = 6
normalGlyphs1Display.GlyphType.TipRadius = 0.1
normalGlyphs1Display.GlyphType.TipLength = 0.35
normalGlyphs1Display.GlyphType.ShaftResolution = 6
normalGlyphs1Display.GlyphType.ShaftRadius = 0.03
normalGlyphs1Display.GlyphType.Invert = 0

# init the 'GridAxesRepresentation' selected for 'DataAxesGrid'
normalGlyphs1Display.DataAxesGrid.XTitle = 'X Axis'
normalGlyphs1Display.DataAxesGrid.YTitle = 'Y Axis'
normalGlyphs1Display.DataAxesGrid.ZTitle = 'Z Axis'
normalGlyphs1Display.DataAxesGrid.XTitleColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.XTitleFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.XTitleBold = 0
normalGlyphs1Display.DataAxesGrid.XTitleItalic = 0
normalGlyphs1Display.DataAxesGrid.XTitleFontSize = 12
normalGlyphs1Display.DataAxesGrid.XTitleShadow = 0
normalGlyphs1Display.DataAxesGrid.XTitleOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.YTitleColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.YTitleFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.YTitleBold = 0
normalGlyphs1Display.DataAxesGrid.YTitleItalic = 0
normalGlyphs1Display.DataAxesGrid.YTitleFontSize = 12
normalGlyphs1Display.DataAxesGrid.YTitleShadow = 0
normalGlyphs1Display.DataAxesGrid.YTitleOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.ZTitleColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.ZTitleFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.ZTitleBold = 0
normalGlyphs1Display.DataAxesGrid.ZTitleItalic = 0
normalGlyphs1Display.DataAxesGrid.ZTitleFontSize = 12
normalGlyphs1Display.DataAxesGrid.ZTitleShadow = 0
normalGlyphs1Display.DataAxesGrid.ZTitleOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.FacesToRender = 63
normalGlyphs1Display.DataAxesGrid.CullBackface = 0
normalGlyphs1Display.DataAxesGrid.CullFrontface = 1
normalGlyphs1Display.DataAxesGrid.GridColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.ShowGrid = 0
normalGlyphs1Display.DataAxesGrid.ShowEdges = 1
normalGlyphs1Display.DataAxesGrid.ShowTicks = 1
normalGlyphs1Display.DataAxesGrid.LabelUniqueEdgesOnly = 1
normalGlyphs1Display.DataAxesGrid.AxesToLabel = 63
normalGlyphs1Display.DataAxesGrid.XLabelColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.XLabelFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.XLabelBold = 0
normalGlyphs1Display.DataAxesGrid.XLabelItalic = 0
normalGlyphs1Display.DataAxesGrid.XLabelFontSize = 12
normalGlyphs1Display.DataAxesGrid.XLabelShadow = 0
normalGlyphs1Display.DataAxesGrid.XLabelOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.YLabelColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.YLabelFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.YLabelBold = 0
normalGlyphs1Display.DataAxesGrid.YLabelItalic = 0
normalGlyphs1Display.DataAxesGrid.YLabelFontSize = 12
normalGlyphs1Display.DataAxesGrid.YLabelShadow = 0
normalGlyphs1Display.DataAxesGrid.YLabelOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.ZLabelColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.DataAxesGrid.ZLabelFontFamily = 'Arial'
normalGlyphs1Display.DataAxesGrid.ZLabelBold = 0
normalGlyphs1Display.DataAxesGrid.ZLabelItalic = 0
normalGlyphs1Display.DataAxesGrid.ZLabelFontSize = 12
normalGlyphs1Display.DataAxesGrid.ZLabelShadow = 0
normalGlyphs1Display.DataAxesGrid.ZLabelOpacity = 1.0
normalGlyphs1Display.DataAxesGrid.XAxisNotation = 'Mixed'
normalGlyphs1Display.DataAxesGrid.XAxisPrecision = 2
normalGlyphs1Display.DataAxesGrid.XAxisUseCustomLabels = 0
normalGlyphs1Display.DataAxesGrid.XAxisLabels = []
normalGlyphs1Display.DataAxesGrid.YAxisNotation = 'Mixed'
normalGlyphs1Display.DataAxesGrid.YAxisPrecision = 2
normalGlyphs1Display.DataAxesGrid.YAxisUseCustomLabels = 0
normalGlyphs1Display.DataAxesGrid.YAxisLabels = []
normalGlyphs1Display.DataAxesGrid.ZAxisNotation = 'Mixed'
normalGlyphs1Display.DataAxesGrid.ZAxisPrecision = 2
normalGlyphs1Display.DataAxesGrid.ZAxisUseCustomLabels = 0
normalGlyphs1Display.DataAxesGrid.ZAxisLabels = []

# init the 'PolarAxesRepresentation' selected for 'PolarAxes'
normalGlyphs1Display.PolarAxes.Visibility = 0
normalGlyphs1Display.PolarAxes.Translation = [0.0, 0.0, 0.0]
normalGlyphs1Display.PolarAxes.Scale = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.Orientation = [0.0, 0.0, 0.0]
normalGlyphs1Display.PolarAxes.EnableCustomBounds = [0, 0, 0]
normalGlyphs1Display.PolarAxes.CustomBounds = [0.0, 1.0, 0.0, 1.0, 0.0, 1.0]
normalGlyphs1Display.PolarAxes.EnableCustomRange = 0
normalGlyphs1Display.PolarAxes.CustomRange = [0.0, 1.0]
normalGlyphs1Display.PolarAxes.PolarAxisVisibility = 1
normalGlyphs1Display.PolarAxes.RadialAxesVisibility = 1
normalGlyphs1Display.PolarAxes.DrawRadialGridlines = 1
normalGlyphs1Display.PolarAxes.PolarArcsVisibility = 1
normalGlyphs1Display.PolarAxes.DrawPolarArcsGridlines = 1
normalGlyphs1Display.PolarAxes.NumberOfRadialAxes = 0
normalGlyphs1Display.PolarAxes.AutoSubdividePolarAxis = 1
normalGlyphs1Display.PolarAxes.NumberOfPolarAxis = 0
normalGlyphs1Display.PolarAxes.MinimumRadius = 0.0
normalGlyphs1Display.PolarAxes.MinimumAngle = 0.0
normalGlyphs1Display.PolarAxes.MaximumAngle = 90.0
normalGlyphs1Display.PolarAxes.RadialAxesOriginToPolarAxis = 1
normalGlyphs1Display.PolarAxes.Ratio = 1.0
normalGlyphs1Display.PolarAxes.PolarAxisColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.PolarArcsColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.LastRadialAxisColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.SecondaryPolarArcsColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.PolarAxisTitleVisibility = 1
normalGlyphs1Display.PolarAxes.PolarAxisTitle = 'Radial Distance'
normalGlyphs1Display.PolarAxes.PolarAxisTitleLocation = 'Bottom'
normalGlyphs1Display.PolarAxes.PolarLabelVisibility = 1
normalGlyphs1Display.PolarAxes.PolarLabelFormat = '%-#6.3g'
normalGlyphs1Display.PolarAxes.PolarLabelExponentLocation = 'Labels'
normalGlyphs1Display.PolarAxes.RadialLabelVisibility = 1
normalGlyphs1Display.PolarAxes.RadialLabelFormat = '%-#3.1f'
normalGlyphs1Display.PolarAxes.RadialLabelLocation = 'Bottom'
normalGlyphs1Display.PolarAxes.RadialUnitsVisibility = 1
normalGlyphs1Display.PolarAxes.ScreenSize = 10.0
normalGlyphs1Display.PolarAxes.PolarAxisTitleColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.PolarAxisTitleOpacity = 1.0
normalGlyphs1Display.PolarAxes.PolarAxisTitleFontFamily = 'Arial'
normalGlyphs1Display.PolarAxes.PolarAxisTitleBold = 0
normalGlyphs1Display.PolarAxes.PolarAxisTitleItalic = 0
normalGlyphs1Display.PolarAxes.PolarAxisTitleShadow = 0
normalGlyphs1Display.PolarAxes.PolarAxisTitleFontSize = 12
normalGlyphs1Display.PolarAxes.PolarAxisLabelColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.PolarAxisLabelOpacity = 1.0
normalGlyphs1Display.PolarAxes.PolarAxisLabelFontFamily = 'Arial'
normalGlyphs1Display.PolarAxes.PolarAxisLabelBold = 0
normalGlyphs1Display.PolarAxes.PolarAxisLabelItalic = 0
normalGlyphs1Display.PolarAxes.PolarAxisLabelShadow = 0
normalGlyphs1Display.PolarAxes.PolarAxisLabelFontSize = 12
normalGlyphs1Display.PolarAxes.LastRadialAxisTextColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.LastRadialAxisTextOpacity = 1.0
normalGlyphs1Display.PolarAxes.LastRadialAxisTextFontFamily = 'Arial'
normalGlyphs1Display.PolarAxes.LastRadialAxisTextBold = 0
normalGlyphs1Display.PolarAxes.LastRadialAxisTextItalic = 0
normalGlyphs1Display.PolarAxes.LastRadialAxisTextShadow = 0
normalGlyphs1Display.PolarAxes.LastRadialAxisTextFontSize = 12
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextColor = [1.0, 1.0, 1.0]
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextOpacity = 1.0
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextFontFamily = 'Arial'
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextBold = 0
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextItalic = 0
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextShadow = 0
normalGlyphs1Display.PolarAxes.SecondaryRadialAxesTextFontSize = 12
normalGlyphs1Display.PolarAxes.EnableDistanceLOD = 1
normalGlyphs1Display.PolarAxes.DistanceLODThreshold = 0.7
normalGlyphs1Display.PolarAxes.EnableViewAngleLOD = 1
normalGlyphs1Display.PolarAxes.ViewAngleLODThreshold = 0.7
normalGlyphs1Display.PolarAxes.SmallestVisiblePolarAngle = 0.5
normalGlyphs1Display.PolarAxes.PolarTicksVisibility = 1
normalGlyphs1Display.PolarAxes.ArcTicksOriginToPolarAxis = 1
normalGlyphs1Display.PolarAxes.TickLocation = 'Both'
normalGlyphs1Display.PolarAxes.AxisTickVisibility = 1
normalGlyphs1Display.PolarAxes.AxisMinorTickVisibility = 0
normalGlyphs1Display.PolarAxes.ArcTickVisibility = 1
normalGlyphs1Display.PolarAxes.ArcMinorTickVisibility = 0
normalGlyphs1Display.PolarAxes.DeltaAngleMajor = 10.0
normalGlyphs1Display.PolarAxes.DeltaAngleMinor = 5.0
normalGlyphs1Display.PolarAxes.PolarAxisMajorTickSize = 0.0
normalGlyphs1Display.PolarAxes.PolarAxisTickRatioSize = 0.3
normalGlyphs1Display.PolarAxes.PolarAxisMajorTickThickness = 1.0
normalGlyphs1Display.PolarAxes.PolarAxisTickRatioThickness = 0.5
normalGlyphs1Display.PolarAxes.LastRadialAxisMajorTickSize = 0.0
normalGlyphs1Display.PolarAxes.LastRadialAxisTickRatioSize = 0.3
normalGlyphs1Display.PolarAxes.LastRadialAxisMajorTickThickness = 1.0
normalGlyphs1Display.PolarAxes.LastRadialAxisTickRatioThickness = 0.5
normalGlyphs1Display.PolarAxes.ArcMajorTickSize = 0.0
normalGlyphs1Display.PolarAxes.ArcTickRatioSize = 0.3
normalGlyphs1Display.PolarAxes.ArcMajorTickThickness = 1.0
normalGlyphs1Display.PolarAxes.ArcTickRatioThickness = 0.5
normalGlyphs1Display.PolarAxes.Use2DMode = 0
normalGlyphs1Display.PolarAxes.UseLogAxis = 0

# init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
normalGlyphs1Display.ScaleTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
normalGlyphs1Display.OpacityTransferFunction.Points = [0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0]

# show color bar/color legend
normalGlyphs1Display.SetScalarBarVisibility(renderView1, True)

# update the view to ensure updated data information
renderView1.Update()

# Hide the scalar bar for this color map if no visible data is colored by it.
HideScalarBarIfNotNeeded(sTLSolidLabelingLUT, renderView1)

# change solid color
normalGlyphs1Display.DiffuseColor = [0.3333333333333333, 1.0, 1.0]

#### saving camera placements for all active views

# current camera placement for renderView1
renderView1.CameraPosition = [6.028750419616699, 6.198199272155762, 211.28633232581421]
renderView1.CameraFocalPoint = [6.028750419616699, 6.198199272155762, -0.8318500518798828]
renderView1.CameraParallelScale = 54.90022541187713

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).