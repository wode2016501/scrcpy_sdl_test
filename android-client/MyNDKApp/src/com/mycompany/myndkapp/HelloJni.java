/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.mycompany.myndkapp;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;


public class HelloJni extends Activity
{
	static final String TAG = "尼玛java";
	private SurfaceView surfaceView=null;
	private Surface surface;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
		

        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
		//不显示程序的标题栏
		requestWindowFeature( Window.FEATURE_NO_TITLE ); 
		 if(surfaceView==null){
			 Log.v(TAG, "创建surfaceView");
        surfaceView = new SurfaceView(this) ;//findViewById(R.id.surfaceView);
		SurfaceHolder surfaceHolder = surfaceView.getHolder();
		surface=surfaceHolder.getSurface(); 
		// surfaceHolder.addCallback(surfaceHolderCallback);
			 surfaceHolder.addCallback(new SurfaceHolder.Callback() {
					 @Override
					 public void surfaceCreated(SurfaceHolder holder) {

						 Log.v(TAG, "surface created.");
						 startPreview(/*holder.getSurface()*/ surface);
					 }

					 @Override
					 public void surfaceDestroyed(SurfaceHolder holder) {
					 }

					 @Override
					 public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
						 Log.v(TAG, "=======format=" + format + " w/h : (" + width + ", " + height + ")");
					 }
				 });
        setContentView(surfaceView);
      //  startPreview(surface); 
		}
       // setContentView(surfaceView);
    }

    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public static native void startPreview(Surface surface);

    /* This is another native method declaration that is *not*
     * implemented by 'hello-jni'. This is simply to show that
     * you can declare as many native methods in your Java code
     * as you want, their implementation is searched in the
     * currently loaded native libraries only the first time
     * you call them.
     *
     * Trying to call this function will result in a
     * java.lang.UnsatisfiedLinkError exception !
     */
  //  public native String  unimplementedStringFromJNI();

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.hellojni/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("hello-jni");
    }
	@Override
	protected void onPause() {
		//super.onPause();
	}

	@Override
	protected void onDestroy() {
		// stopPreview();
		//super.onDestroy();
	}
	
}
