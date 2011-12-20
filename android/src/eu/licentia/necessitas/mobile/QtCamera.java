/*
Copyright (c) 2011 Elektrobit (EB), All rights reserved.
Contact: oss-devel@elektrobit.com

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are
met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the Elektrobit (EB) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Elektrobit (EB) ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Elektrobit (EB) BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package eu.licentia.necessitas.mobile;

//@ANDROID-8
//QtCreator import java.io.ByteArrayOutputStream;
//QtCreator import java.io.IOException;
//QtCreator 
//QtCreator import android.app.Activity;
//QtCreator import android.content.BroadcastReceiver;
//QtCreator import android.content.Context;
//QtCreator import android.content.Intent;
//QtCreator import android.content.IntentFilter;
//QtCreator import android.graphics.Bitmap;
//QtCreator import android.graphics.Bitmap.CompressFormat;
//QtCreator import android.graphics.BitmapFactory;
//QtCreator import android.graphics.ImageFormat;
//QtCreator import android.graphics.Rect;
//QtCreator import android.graphics.YuvImage;
//QtCreator import android.hardware.Camera;
//QtCreator import android.hardware.Camera.Parameters;
//QtCreator import android.hardware.Camera.PictureCallback;
//QtCreator import android.hardware.Camera.PreviewCallback;
//QtCreator import android.hardware.Camera.ShutterCallback;
//QtCreator import android.hardware.Camera.Size;
//QtCreator import android.media.MediaRecorder;
//QtCreator import android.util.Log;
//QtCreator import android.view.SurfaceHolder;
//QtCreator import android.view.SurfaceHolder.Callback;
//QtCreator import android.view.SurfaceView;
//QtCreator import eu.licentia.necessitas.industrius.QtApplication;
//QtCreator import eu.licentia.necessitas.industrius.QtLayout;
//QtCreator 
//QtCreator public class QtCamera implements PreviewCallback, Callback{
//QtCreator     private static Camera m_camera;
//QtCreator     public ShutterCallback shutterCallback;
//QtCreator     public PictureCallback rawCallback;
//QtCreator     public PictureCallback jpegCallback;
//QtCreator     private static Activity m_activity = null;
//QtCreator     String[] m_sceneList;
//QtCreator     String[] m_focusModes;
//QtCreator     String[] m_flashModes;
//QtCreator     String[] m_whiteBalanceModes;
//QtCreator     static String m_currentFocusMode;
//QtCreator     int[] m_imageFormats;
//QtCreator     int[] m_imageResolutions;
//QtCreator     private Parameters m_params;
//QtCreator     public static PreviewCallback m_previewCallback;
//QtCreator     private int m_width;
//QtCreator     private int m_height;
//QtCreator     public static MediaRecorder m_recorder=null;
//QtCreator     private SurfaceView m_surfaceView;
//QtCreator     private String m_videoOutputPath = null;
//QtCreator     private int m_videoOutFormat = MediaRecorder.OutputFormat.MPEG_4;
//QtCreator     private int m_videoFrameRate = 30;
//QtCreator     private int[] m_videoFramesize = new int[2];
//QtCreator     private long m_maxVideoFileSize=0;
//QtCreator     private int m_videoEncodingBitrate=0;
//QtCreator     private int m_audioBitRate=0;
//QtCreator     private int m_audioChannelsCount=0;
//QtCreator     int[] m_videoPreviewParams;
//QtCreator     public boolean m_screenOff = false;
//QtCreator     private int m_surfaceDestroyedOff = 0;
//QtCreator     public boolean m_surfaceDestroyed = false;
//QtCreator     QtCamera()
//QtCreator     {
//QtCreator         setActivity();
//QtCreator         m_previewCallback = this;
//QtCreator         m_videoFramesize[0] = 480;
//QtCreator         m_videoFramesize[1] = 360;
//QtCreator         m_videoPreviewParams = new int[4];
//QtCreator         IntentFilter filter = new IntentFilter(Intent.ACTION_SCREEN_OFF);
//QtCreator         BroadcastReceiver mReceiver = new ScreenReceiver();
//QtCreator         QtApplication.mainActivity().registerReceiver(mReceiver, filter);
//QtCreator         m_surfaceView = new SurfaceView(QtApplication.mainActivity());
//QtCreator         m_surfaceView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
//QtCreator         m_surfaceView.getHolder().addCallback(this);
//QtCreator         m_surfaceView.setFocusable(true);
//QtCreator         QtApplication.mainActivity().getQtLayout().addView(m_surfaceView,1,new QtLayout.LayoutParams(0,0,1,1));
//QtCreator     }
//QtCreator 
//QtCreator     public static Camera getCamera()
//QtCreator     {
//QtCreator         return m_camera;
//QtCreator     }
//QtCreator 
//QtCreator     public void setOutputFile(String filename)
//QtCreator     {
//QtCreator         m_videoOutputPath = filename;
//QtCreator     }
//QtCreator 
//QtCreator     public void setOutputFormat(int format)
//QtCreator     {
//QtCreator         m_videoOutFormat = format;
//QtCreator     }
//QtCreator 
//QtCreator     public void setVideoEncodingBitrate(int rate)
//QtCreator     {
//QtCreator         m_videoEncodingBitrate = rate;
//QtCreator     }
//QtCreator 
//QtCreator     public void setMaxVideoSize(long size)
//QtCreator     {
//QtCreator         m_maxVideoFileSize = size;
//QtCreator     }
//QtCreator 
//QtCreator     public void setVideoSettings(int[] settings)
//QtCreator     {
//QtCreator         m_videoFrameRate = settings[0];
//QtCreator         m_videoFramesize[0] = settings[1];
//QtCreator         m_videoFramesize[1] = settings[2];
//QtCreator     }
//QtCreator 
//QtCreator     public void setAudioBitRate(int rate)
//QtCreator     {
//QtCreator         m_audioBitRate = rate;
//QtCreator     }
//QtCreator 
//QtCreator 
//QtCreator     public void setAudioChannelsCount(int count)
//QtCreator     {
//QtCreator         m_audioChannelsCount = count;
//QtCreator     }
//QtCreator 
//QtCreator     public void startRecording()
//QtCreator     {
//QtCreator         m_activity.runOnUiThread(new Runnable() {
//QtCreator             @Override
//QtCreator             public void run() {
//QtCreator                 QtApplication.mainActivity().getQtLayout().updateViewLayout(m_surfaceView, new QtLayout.LayoutParams(m_videoPreviewParams[0],m_videoPreviewParams[1],m_videoPreviewParams[2],m_videoPreviewParams[3]));
//QtCreator             }
//QtCreator         });
//QtCreator         m_camera.stopPreview();
//QtCreator         m_camera.unlock();
//QtCreator 
//QtCreator         if(m_recorder == null)
//QtCreator         {
//QtCreator             m_recorder = new MediaRecorder();
//QtCreator         }
//QtCreator         m_recorder.setCamera(m_camera);
//QtCreator         m_recorder.setAudioSource(MediaRecorder.AudioSource.MIC);
//QtCreator         m_recorder.setVideoSource(MediaRecorder.VideoSource.DEFAULT);
//QtCreator         m_recorder.setOutputFormat(m_videoOutFormat);
//QtCreator 
//QtCreator         long currentDateTimeString = System.currentTimeMillis();
//QtCreator         String filePath;
//QtCreator 
//QtCreator         // WARNING unsafe hardcoded path !!!
//QtCreator         if(m_videoOutputPath == null)
//QtCreator         {
//QtCreator             if(m_videoOutFormat == 1)
//QtCreator             {
//QtCreator                 filePath = "/sdcard/"+currentDateTimeString+".3gp";
//QtCreator             }
//QtCreator             else
//QtCreator             {
//QtCreator                 filePath = "/sdcard/"+currentDateTimeString+".mp4";
//QtCreator             }
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             if(m_videoOutFormat == 1)
//QtCreator             {
//QtCreator                 filePath = m_videoOutputPath+currentDateTimeString+".3gp";
//QtCreator             }
//QtCreator             else
//QtCreator             {
//QtCreator                 filePath = m_videoOutputPath+currentDateTimeString+".mp4";
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         m_recorder.setOutputFile(filePath);
//QtCreator 
//QtCreator         if(m_maxVideoFileSize != 0)
//QtCreator         {
//QtCreator             m_recorder.setMaxFileSize(m_maxVideoFileSize);
//QtCreator         }
//QtCreator         m_recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);
//QtCreator         m_recorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
//QtCreator         m_recorder.setVideoFrameRate(m_videoFrameRate);
//QtCreator         m_recorder.setVideoSize(m_videoFramesize[0], m_videoFramesize[1]);
//QtCreator 
//QtCreator         if(m_videoEncodingBitrate != 0)
//QtCreator         {
//QtCreator             m_recorder.setVideoEncodingBitRate(m_videoEncodingBitrate);
//QtCreator         }
//QtCreator 
//QtCreator         if(m_audioBitRate != 0)
//QtCreator         {
//QtCreator             m_recorder.setAudioEncodingBitRate(m_audioBitRate);
//QtCreator         }
//QtCreator 
//QtCreator         if(m_audioChannelsCount != 0)
//QtCreator         {
//QtCreator             m_recorder.setAudioChannels(m_audioChannelsCount);
//QtCreator         }
//QtCreator 
//QtCreator         m_recorder.setPreviewDisplay(m_surfaceView.getHolder().getSurface());
//QtCreator         if (m_recorder != null) {
//QtCreator             try {
//QtCreator                 m_recorder.prepare();
//QtCreator                 m_recorder.start();
//QtCreator             } catch (IllegalStateException e) {
//QtCreator                 e.printStackTrace();
//QtCreator             } catch (IOException e) {
//QtCreator                 e.printStackTrace();
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public void stopRecording()
//QtCreator     {
//QtCreator         Log.i("Stop Record  called", "in stopRecording");
//QtCreator         if(m_recorder != null)
//QtCreator         {
//QtCreator 
//QtCreator             m_recorder.stop();
//QtCreator             m_recorder.reset();
//QtCreator             m_recorder.release();
//QtCreator             m_recorder = null;
//QtCreator         }
//QtCreator         m_activity.runOnUiThread(new Runnable() {
//QtCreator             public void run() {
//QtCreator                 QtApplication.mainActivity().getQtLayout().updateViewLayout(m_surfaceView, new QtLayout.LayoutParams(0,0,1,1));
//QtCreator             }
//QtCreator         });
//QtCreator 
//QtCreator         try {
//QtCreator             m_camera.reconnect();
//QtCreator         } catch (IOException e) {
//QtCreator             e.printStackTrace();
//QtCreator         }
//QtCreator         m_params = m_camera.getParameters();
//QtCreator         m_params.setPreviewSize(m_width,m_height);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator         m_camera.setPreviewCallback(this);
//QtCreator         try {
//QtCreator             m_camera.setPreviewDisplay(null);
//QtCreator         } catch (IOException e) {
//QtCreator             e.printStackTrace();
//QtCreator         }
//QtCreator         m_camera.startPreview();
//QtCreator     }
//QtCreator 
//QtCreator 
//QtCreator     public static void setActivity()
//QtCreator     {
//QtCreator         m_activity = QtApplication.mainActivity();
//QtCreator     }
//QtCreator 
//QtCreator     public void setCameraState(int state)
//QtCreator     {
//QtCreator         switch(state)
//QtCreator         {
//QtCreator 
//QtCreator         case 0:
//QtCreator             openCamera();
//QtCreator             m_params = m_camera.getParameters();
//QtCreator             m_params.setPreviewFormat(ImageFormat.NV21);
//QtCreator             m_camera.setParameters(m_params);
//QtCreator             getSupportedModes();
//QtCreator             getSupportedImageFormats();
//QtCreator             getSupportedImageResolutions();
//QtCreator             break;
//QtCreator 
//QtCreator         case 1:
//QtCreator             m_camera.release();
//QtCreator             m_camera = null;
//QtCreator             break;
//QtCreator 
//QtCreator         case 2:
//QtCreator             Log.i("tag", "stopping the preview************");
//QtCreator             m_camera.setPreviewCallback(this);
//QtCreator             try {
//QtCreator                     m_camera.setPreviewDisplay(null);
//QtCreator             } catch (IOException e) {
//QtCreator                     e.printStackTrace();
//QtCreator             }
//QtCreator             m_camera.startPreview();
//QtCreator             callBacks();
//QtCreator             startFocus();
//QtCreator             break;
//QtCreator 
//QtCreator         case 3:
//QtCreator             stopFocus();
//QtCreator             m_camera.stopPreview();
//QtCreator             break;
//QtCreator 
//QtCreator         default:
//QtCreator                 break;
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public static void openCamera()
//QtCreator     {
//QtCreator         if(m_camera == null)
//QtCreator         {
//QtCreator             m_camera = Camera.open();
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public void takePicture()
//QtCreator     {
//QtCreator         m_camera.stopPreview();
//QtCreator         m_camera.takePicture(null,null,jpegCallback);
//QtCreator     }
//QtCreator 
//QtCreator     public void callBacks()
//QtCreator     {
//QtCreator         /** Handles data for jpeg picture */
//QtCreator         jpegCallback = new PictureCallback() {
//QtCreator             public void onPictureTaken(byte[] data, Camera camera) {
//QtCreator                 getImage(data);
//QtCreator                 m_camera.setPreviewCallback(m_previewCallback);
//QtCreator                 m_camera.startPreview();
//QtCreator                 Log.i("Camera", "onPictureTaken - jpeg"+m_params.getPictureSize().height+m_params.getPictureSize().width);
//QtCreator             }
//QtCreator         };
//QtCreator     }
//QtCreator 
//QtCreator     public void setSceneMode(String mode)
//QtCreator     {
//QtCreator         m_params.setSceneMode(mode);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public int[] getCompensationRange()
//QtCreator     {
//QtCreator         int[] range = {0,0};
//QtCreator         range[0] = (int)(m_params.getMinExposureCompensation() * m_params.getExposureCompensationStep());
//QtCreator         range[1] = (int)(m_params.getMaxExposureCompensation() * m_params.getExposureCompensationStep());
//QtCreator         return range;
//QtCreator     }
//QtCreator 
//QtCreator     public void setCompensation(int value)
//QtCreator     {
//QtCreator         int compensationIndex =(int) (value/m_params.getExposureCompensationStep());
//QtCreator         m_params.setExposureCompensation(compensationIndex);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public void setFocusMode(String mode)
//QtCreator     {
//QtCreator         m_currentFocusMode = mode;
//QtCreator         m_params.setFocusMode(mode);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public void startFocus()
//QtCreator     {
//QtCreator         if(m_currentFocusMode != null)
//QtCreator         {
//QtCreator             if(m_currentFocusMode.contains("auto" ) || m_currentFocusMode.contains("macro"))
//QtCreator             {
//QtCreator                 m_camera.autoFocus(null);
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public void stopFocus()
//QtCreator     {
//QtCreator         if(m_currentFocusMode != null)
//QtCreator         {
//QtCreator             if(m_currentFocusMode.contains("auto" ) || m_currentFocusMode.contains("macro"))
//QtCreator             {
//QtCreator                 m_camera.cancelAutoFocus();
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator 
//QtCreator     public void getSupportedModes()
//QtCreator     {
//QtCreator         m_sceneList = new String[m_params.getSupportedSceneModes().size()];
//QtCreator         for(int i =0;i < m_params.getSupportedSceneModes().size();i++ )
//QtCreator         {
//QtCreator             m_sceneList[i] = m_params.getSupportedSceneModes().get(i);
//QtCreator         }
//QtCreator 
//QtCreator 
//QtCreator         m_focusModes = new String[m_params.getSupportedFocusModes().size()];
//QtCreator         if(m_params.getSupportedFocusModes() == null)
//QtCreator         {
//QtCreator             m_focusModes[0] = "Not Supported";
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             for(int i =0;i < m_params.getSupportedFocusModes().size();i++ )
//QtCreator             {
//QtCreator                 m_focusModes[i] = "";
//QtCreator                 m_focusModes[i] = m_params.getSupportedFocusModes().get(i);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         m_flashModes = new String[m_params.getSupportedFlashModes().size()];
//QtCreator         if(m_params.getSupportedFlashModes() == null)
//QtCreator         {
//QtCreator             m_flashModes[0] = "Not Supported";
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             for(int i =0;i < m_params.getSupportedFlashModes().size();i++ )
//QtCreator             {
//QtCreator                 m_flashModes[i] = m_params.getSupportedFlashModes().get(i);
//QtCreator             }
//QtCreator         }
//QtCreator 
//QtCreator         m_whiteBalanceModes = new String[m_params.getSupportedWhiteBalance().size()];
//QtCreator         if(m_params.getSupportedWhiteBalance() == null)
//QtCreator         {
//QtCreator             m_whiteBalanceModes[0] = "Not Supported";
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator             for(int i =0;i < m_params.getSupportedWhiteBalance().size();i++ )
//QtCreator             {
//QtCreator                 m_whiteBalanceModes[i] = m_params.getSupportedWhiteBalance().get(i);
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public int[] getSupportedImageResolutions()
//QtCreator     {
//QtCreator         m_imageResolutions = new int[2*m_params.getSupportedPictureSizes().size()];
//QtCreator         for(int i =0;i < m_params.getSupportedPictureSizes().size();i=i+2 )
//QtCreator         {
//QtCreator             Size size = m_params.getSupportedPictureSizes().get(i);
//QtCreator             m_imageResolutions[i] = size.width;
//QtCreator             m_imageResolutions[i+1] = size.height;
//QtCreator         }
//QtCreator         return m_imageResolutions;
//QtCreator     }
//QtCreator 
//QtCreator     public int[] getSupportedImageFormats()
//QtCreator     {
//QtCreator         m_imageFormats = new int[m_params.getSupportedPictureFormats().size()];
//QtCreator         for(int i =0;i < m_params.getSupportedPictureFormats().size();i++ )
//QtCreator         {
//QtCreator             m_imageFormats[i] = m_params.getSupportedPictureFormats().get(i);
//QtCreator         }
//QtCreator         return m_imageFormats;
//QtCreator     }
//QtCreator 
//QtCreator 
//QtCreator     public int getMaxZoom()
//QtCreator     {
//QtCreator         if(m_params.isZoomSupported())
//QtCreator         {
//QtCreator             return m_params.getMaxZoom();
//QtCreator         }
//QtCreator         return 0;
//QtCreator     }
//QtCreator 
//QtCreator     public int getZoom()
//QtCreator     {
//QtCreator         if(m_params.isZoomSupported())
//QtCreator         {
//QtCreator             return m_params.getZoom();
//QtCreator         }
//QtCreator         return 0;
//QtCreator     }
//QtCreator 
//QtCreator     public void setZoom(int zoom)
//QtCreator     {
//QtCreator         if(m_params.isZoomSupported())
//QtCreator         {
//QtCreator             m_params.setZoom(zoom);
//QtCreator             m_camera.setParameters(m_params);
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     public void setFlashMode(String mode)
//QtCreator     {
//QtCreator         m_params.setFlashMode(mode);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public void setWhiteBalanceMode(String mode)
//QtCreator     {
//QtCreator         m_params.setWhiteBalance(mode);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public void setImageSettings(int[] settings)
//QtCreator     {
//QtCreator         m_params.setPictureFormat(settings[0]);
//QtCreator         m_params.setPictureSize(settings[1],settings[2]);
//QtCreator         m_camera.setParameters(m_params);
//QtCreator     }
//QtCreator 
//QtCreator     public void onPreviewFrame(byte[] data, Camera camera) {
//QtCreator         m_width= m_params.getPreviewSize().width;
//QtCreator         m_height = m_params.getPreviewSize().height;
//QtCreator         ByteArrayOutputStream output_stream = new ByteArrayOutputStream();
//QtCreator         YuvImage image =new YuvImage(data,ImageFormat.NV21,m_width,m_height,null);
//QtCreator         image.compressToJpeg(new Rect(0,0,m_width,m_height),60, output_stream);
//QtCreator         Bitmap bitmap = BitmapFactory.decodeByteArray(output_stream.toByteArray(), 0, output_stream.size());
//QtCreator         Bitmap scaledBitmap = Bitmap.createScaledBitmap(bitmap, m_width/2, m_height/2, true);//Optimization of preview data sent
//QtCreator         ByteArrayOutputStream imageStream = new ByteArrayOutputStream();
//QtCreator         scaledBitmap.compress(CompressFormat.JPEG,60, imageStream);
//QtCreator         getPreviewBuffers(imageStream.toByteArray());
//QtCreator     }
//QtCreator 
//QtCreator     public static native void getImage(byte[] data);
//QtCreator     public static native void getPreviewBuffers(byte[] data);
//QtCreator     public static native void stopRecord();
//QtCreator 
//QtCreator 
//QtCreator     public class ScreenReceiver extends BroadcastReceiver {
//QtCreator         @Override
//QtCreator         public void onReceive(Context context, Intent intent) {
//QtCreator             if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF))
//QtCreator             {
//QtCreator                 if(m_recorder != null)
//QtCreator                 {
//QtCreator                     m_recorder.stop();
//QtCreator                     m_recorder.reset();
//QtCreator                     m_recorder.release();
//QtCreator                     m_recorder = null;
//QtCreator                     stopRecord();
//QtCreator                 }
//QtCreator                 m_screenOff = true;
//QtCreator                 QtApplication.mainActivity().unregisterReceiver(this);
//QtCreator             }
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     @Override
//QtCreator     public void surfaceChanged(SurfaceHolder holder, int format, int width,
//QtCreator             int height)
//QtCreator     {
//QtCreator         if((m_screenOff == true && m_surfaceDestroyedOff == 2) || m_surfaceDestroyed == true)
//QtCreator         {
//QtCreator             m_camera = QtCamera.getCamera();
//QtCreator             if (m_camera!=null)
//QtCreator             {
//QtCreator                 try {
//QtCreator                         m_camera.reconnect();
//QtCreator                 } catch (IOException e) {
//QtCreator                         e.printStackTrace();
//QtCreator                 }
//QtCreator                 m_params = m_camera.getParameters();
//QtCreator                 m_params.setPreviewSize(720,480);
//QtCreator                 m_camera.setParameters(m_params);
//QtCreator                 m_camera.setPreviewCallback(QtCamera.m_previewCallback);
//QtCreator                 try {
//QtCreator                     m_camera.setPreviewDisplay(null);
//QtCreator                 } catch (IOException e) {
//QtCreator                         e.printStackTrace();
//QtCreator                 }
//QtCreator                 m_camera.startPreview();
//QtCreator             }
//QtCreator             m_screenOff = false;
//QtCreator             m_surfaceDestroyedOff = 0;
//QtCreator             m_surfaceDestroyed = false;
//QtCreator         }
//QtCreator     }
//QtCreator 
//QtCreator     @Override
//QtCreator     public void surfaceCreated(SurfaceHolder holder) {
//QtCreator     // TODO Auto-generated method stub
//QtCreator 
//QtCreator     }
//QtCreator 
//QtCreator     @Override
//QtCreator     public void surfaceDestroyed(SurfaceHolder holder) {
//QtCreator 
//QtCreator         if(m_screenOff == true)
//QtCreator         {
//QtCreator                 m_surfaceDestroyedOff++;
//QtCreator         }
//QtCreator         else
//QtCreator         {
//QtCreator                 m_surfaceDestroyed = true;
//QtCreator                 if(QtCamera.m_recorder != null)
//QtCreator                 {
//QtCreator                         QtCamera.m_recorder.stop();
//QtCreator                         QtCamera.m_recorder.reset();
//QtCreator                         QtCamera.m_recorder.release();
//QtCreator                         QtCamera.m_recorder = null;
//QtCreator                         QtCamera.stopRecord();
//QtCreator                 }
//QtCreator         }
//QtCreator     }
//QtCreator }
//@ANDROID-8
