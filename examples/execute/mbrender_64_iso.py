
#--------------------------------------------------------------

# Global timestep output options
timeStepToStartOutputAt=0
forceOutputAtFirstCall=False

# Global screenshot output options
imageFileNamePadding=0
rescale_lookuptable=False

# Whether or not to request specific arrays from the adaptor.
requestSpecificArrays=False

# a root directory under which all Catalyst output goes
rootDirectory=''

# makes a cinema D index table
make_cinema_table=False

#--------------------------------------------------------------
# Code generated from cpstate.py to create the CoProcessor.
# paraview version 5.8.0
#--------------------------------------------------------------

from paraview.simple import *
from paraview import coprocessing

# ----------------------- CoProcessor definition -----------------------

def CreateCoProcessor():
  def _CreatePipeline(coprocessor, datadescription):
    class Pipeline:
      # state file generated using paraview version 5.8.0

      # ----------------------------------------------------------------
      # setup views used in the visualization
      # ----------------------------------------------------------------

      # trace generated using paraview version 5.8.0
      #
      # To ensure correct image size when batch processing, please search 
      # for and uncomment the line `# renderView*.ViewSize = [*,*]`

      #### disable automatic camera reset on 'Show'
      paraview.simple._DisableFirstRenderCameraReset()

      # get the material library
      materialLibrary1 = GetMaterialLibrary()

      # Create a new 'Render View'
      renderView1 = CreateView('RenderView')
      renderView1.ViewSize = [2107, 1093]
      renderView1.AxesGrid = 'GridAxes3DActor'
      renderView1.CenterOfRotation = [31.999999046325684, 31.99999976158142, 28.05555534362793]
      renderView1.StereoType = 'Crystal Eyes'
      renderView1.CameraPosition = [123.16632643067211, 144.1814149465996, 45.54659367135687]
      renderView1.CameraFocalPoint = [31.99999904632566, 31.999999761581424, 28.05555534362795]
      renderView1.CameraViewUp = [0.32722345087848853, -0.11977898452062072, -0.9373248145986439]
      renderView1.CameraFocalDisk = 1.0
      renderView1.CameraParallelScale = 45.60044165555977
      renderView1.BackEnd = 'OSPRay raycaster'
      renderView1.OSPRayMaterialLibrary = materialLibrary1

      # register the view with coprocessor
      # and provide it with information such as the filename to use,
      # how frequently to write the images, etc.
      coprocessor.RegisterView(renderView1,
          filename='RenderView1_%t.png', freq=1, fittoscreen=0, magnification=1, width=1100, height=1100, cinema={}, compression=5)
      renderView1.ViewTime = datadescription.GetTime()

      SetActiveView(None)

      # ----------------------------------------------------------------
      # setup view layouts
      # ----------------------------------------------------------------

      # create new layout object 'Layout #1'
      layout1 = CreateLayout(name='Layout #1')
      layout1.AssignView(0, renderView1)

      # ----------------------------------------------------------------
      # restore active view
      SetActiveView(renderView1)
      # ----------------------------------------------------------------

      # ----------------------------------------------------------------
      # setup the data processing pipelines
      # ----------------------------------------------------------------

      # create a new 'XML MultiBlock Data Reader'
      # create a producer from a simulation input
      input = coprocessor.CreateProducer(datadescription, 'input')

      # create a new 'Contour'
      contour2 = Contour(Input=input)
      contour2.ContourBy = ['POINTS', 'mandelbulb']
      contour2.Isosurfaces = [50.0]
      contour2.PointMergeMethod = 'Uniform Binning'

      # create a new 'XML MultiBlock Data Reader'
      # create a producer from a simulation input
      input_1 = coprocessor.CreateProducer(datadescription, 'input')

      # create a new 'Contour'
      contour1 = Contour(Input=input_1)
      contour1.ContourBy = ['POINTS', 'grayscottu']
      contour1.Isosurfaces = [0.5918336000470292, 1.0, 0.8, 0.6, 0.4, 0.2]
      contour1.PointMergeMethod = 'Uniform Binning'

      # create a new 'Extract Surface'
      extractSurface1 = ExtractSurface(Input=contour1)

      # create a new 'Clip'
      clip1 = Clip(Input=contour1)
      clip1.ClipType = 'Plane'
      clip1.HyperTreeGridClipper = 'Plane'
      clip1.Scalars = ['POINTS', 'grayscottu']
      clip1.Value = 0.5918335914611816

      # init the 'Plane' selected for 'ClipType'
      clip1.ClipType.Origin = [31.49303436279297, 31.492342948913574, 31.49195957183838]

      # init the 'Plane' selected for 'HyperTreeGridClipper'
      clip1.HyperTreeGridClipper.Origin = [31.49303436279297, 31.492342948913574, 31.49195957183838]

      # ----------------------------------------------------------------
      # setup the visualization in view 'renderView1'
      # ----------------------------------------------------------------

      # show data from contour2
      contour2Display = Show(contour2, renderView1, 'GeometryRepresentation')

      # get color transfer function/color map for 'mandelbulb'
      mandelbulbLUT = GetColorTransferFunction('mandelbulb')
      mandelbulbLUT.RGBPoints = [0.0, 0.231373, 0.298039, 0.752941, 50.0078125, 0.865003, 0.865003, 0.865003, 100.015625, 0.705882, 0.0156863, 0.14902]
      mandelbulbLUT.ScalarRangeInitialized = 1.0

      # trace defaults for the display properties.
      contour2Display.Representation = 'Surface'
      contour2Display.ColorArrayName = ['POINTS', 'mandelbulb']
      contour2Display.LookupTable = mandelbulbLUT
      contour2Display.OSPRayScaleArray = 'mandelbulb'
      contour2Display.OSPRayScaleFunction = 'PiecewiseFunction'
      contour2Display.SelectOrientationVectors = 'None'
      contour2Display.ScaleFactor = 5.903092861175537
      contour2Display.SelectScaleArray = 'mandelbulb'
      contour2Display.GlyphType = 'Arrow'
      contour2Display.GlyphTableIndexArray = 'mandelbulb'
      contour2Display.GaussianRadius = 0.29515464305877687
      contour2Display.SetScaleArray = ['POINTS', 'mandelbulb']
      contour2Display.ScaleTransferFunction = 'PiecewiseFunction'
      contour2Display.OpacityArray = ['POINTS', 'mandelbulb']
      contour2Display.OpacityTransferFunction = 'PiecewiseFunction'
      contour2Display.DataAxesGrid = 'GridAxesRepresentation'
      contour2Display.PolarAxes = 'PolarAxesRepresentation'

      # init the 'PiecewiseFunction' selected for 'ScaleTransferFunction'
      contour2Display.ScaleTransferFunction.Points = [50.0, 0.0, 0.5, 0.0, 50.0078125, 1.0, 0.5, 0.0]

      # init the 'PiecewiseFunction' selected for 'OpacityTransferFunction'
      contour2Display.OpacityTransferFunction.Points = [50.0, 0.0, 0.5, 0.0, 50.0078125, 1.0, 0.5, 0.0]

      # setup the color legend parameters for each legend in this view

      # get color legend/bar for mandelbulbLUT in view renderView1
      mandelbulbLUTColorBar = GetScalarBar(mandelbulbLUT, renderView1)
      mandelbulbLUTColorBar.WindowLocation = 'UpperRightCorner'
      mandelbulbLUTColorBar.Title = 'mandelbulb'
      mandelbulbLUTColorBar.ComponentTitle = ''

      # set color bar visibility
      mandelbulbLUTColorBar.Visibility = 1

      # show color legend
      contour2Display.SetScalarBarVisibility(renderView1, True)

      # ----------------------------------------------------------------
      # setup color maps and opacity mapes used in the visualization
      # note: the Get..() functions create a new object, if needed
      # ----------------------------------------------------------------

      # get opacity transfer function/opacity map for 'mandelbulb'
      mandelbulbPWF = GetOpacityTransferFunction('mandelbulb')
      mandelbulbPWF.Points = [0.0, 0.0, 0.5, 0.0, 100.015625, 1.0, 0.5, 0.0]
      mandelbulbPWF.ScalarRangeInitialized = 1

      # ----------------------------------------------------------------
      # finally, restore active source
      SetActiveSource(contour2)
      # ----------------------------------------------------------------
    return Pipeline()

  class CoProcessor(coprocessing.CoProcessor):
    def CreatePipeline(self, datadescription):
      self.Pipeline = _CreatePipeline(self, datadescription)

  coprocessor = CoProcessor()
  # these are the frequencies at which the coprocessor updates.
  freqs = {'input': [1]}
  coprocessor.SetUpdateFrequencies(freqs)
  if requestSpecificArrays:
    arrays = [['grayscottu', 0]]
    coprocessor.SetRequestedArrays('input', arrays)
  coprocessor.SetInitialOutputOptions(timeStepToStartOutputAt,forceOutputAtFirstCall)

  if rootDirectory:
      coprocessor.SetRootDirectory(rootDirectory)

  if make_cinema_table:
      coprocessor.EnableCinemaDTable()

  return coprocessor


#--------------------------------------------------------------
# Global variable that will hold the pipeline for each timestep
# Creating the CoProcessor object, doesn't actually create the ParaView pipeline.
# It will be automatically setup when coprocessor.UpdateProducers() is called the
# first time.
coprocessor = CreateCoProcessor()

#--------------------------------------------------------------
# Enable Live-Visualizaton with ParaView and the update frequency
coprocessor.EnableLiveVisualization(False, 1)

# ---------------------- Data Selection method ----------------------

def RequestDataDescription(datadescription):
    "Callback to populate the request for current timestep"
    global coprocessor

    # setup requests for all inputs based on the requirements of the
    # pipeline.
    coprocessor.LoadRequestedData(datadescription)

# ------------------------ Processing method ------------------------

def DoCoProcessing(datadescription):
    "Callback to do co-processing for current timestep"
    global coprocessor

    # Update the coprocessor by providing it the newly generated simulation data.
    # If the pipeline hasn't been setup yet, this will setup the pipeline.
    coprocessor.UpdateProducers(datadescription)

    # Write output data, if appropriate.
    coprocessor.WriteData(datadescription);

    # Write image capture (Last arg: rescale lookup table), if appropriate.
    coprocessor.WriteImages(datadescription, rescale_lookuptable=rescale_lookuptable,
        image_quality=0, padding_amount=imageFileNamePadding)

    # Live Visualization, if enabled.
    coprocessor.DoLiveVisualization(datadescription, "localhost", 22222)
