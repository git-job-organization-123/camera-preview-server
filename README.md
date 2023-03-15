# Camera preview server

#### Two client server that reads stream of YUV_420_888 images. 

Example: security camera or video.


Uses OpenGL for rendering.


Reads image data in YUV_420_888 format.

##

#### How the program works:

1. Start the server
2. Wait for a client
3. When connection is made, open camera preview for a client
4. Draw client camera preview to the window
5. Wait for another client
6. When a client connection is closed, stop streaming for the client

##

#### How to send image data:

First, send message that contains image properties separated by space,

then send messages of YUV_420_888 image data as byte[]

First message must contain image properties separated by space in this order:
1. Image width
2. Image height
3. Y plane size
4. UV plane size
5. Y plane row stride
6. Y plane pixel stride
7. UV plane row stride
8. UV plane pixel stride

Example:

`256 256 76800 38399 320 1 320 2`

Java example:

```java
int width = 256;
int height = 256; 

Image image = reader.acquireLatestImage();
Image.Plane[] planes = image.getPlanes();

ByteBuffer yBuffer = planes[0].getBuffer();
ByteBuffer uBuffer = planes[1].getBuffer();
ByteBuffer vBuffer = planes[2].getBuffer();

int ySize = yBuffer.remaining();
int uSize = uBuffer.remaining();
int vSize = vBuffer.remaining();

int yPixelStride = planes[0].getPixelStride();
int yRowStride = planes[0].getRowStride();
int uvPixelStride = planes[1].getPixelStride();
int uvRowStride = planes[1].getRowStride();
```

```java
PrintWriter out = new PrintWriter(serverSocket.getOutputStream(), true);

// Image properties
String message = width + " " +
                 height + " " +
                 ySize + " " + 
                 uSize + " " + // UV size
                 yRowStride + " " +
                 yPixelStride + " " +
                 uvRowStride + " " +
                 uvPixelStride + " ";

// Send image properties to server
out.print(message);
out.flush();

image.close();
```


Other messages must contain combined YUV_420_888 image data as byte[]

Java example:

```java
ImageReader imageReader = ImageReader.newInstance(width, height, ImageFormat.YUV_420_888, 1);
imageReader.setOnImageAvailableListener(
  new ImageReader.OnImageAvailableListener() {
      @Override
      public void onImageAvailable(ImageReader reader) {
        Image image = reader.acquireLatestImage();
        Image.Plane[] planes = image.getPlanes();

        ByteBuffer yBuffer = planes[0].getBuffer();
        ByteBuffer uBuffer = planes[1].getBuffer();
        ByteBuffer vBuffer = planes[2].getBuffer();

        int ySize = yBuffer.remaining();
        int uSize = uBuffer.remaining();
        int vSize = vBuffer.remaining();

        ...

        // Get combined YUV_420_888 image data as byte[]
        byte[] yuv = new byte[ySize + uSize + vSize];
        yBuffer.get(yuv, 0, ySize);
        uBuffer.get(yuv, ySize, uSize);
        vBuffer.get(yuv, ySize + uSize, vSize);

        // Send combined YUV_420_888 image data to server
        serverSocket.getOutputStream().write(yuv);
        serverSocket.getOutputStream().flush();

        image.close();
      }
  });
```

##

## Windows install


#### MinGW64 install

Download Win64 archive:

https://winlibs.com/

Extract it to C:/

Add to PATH:

`C:\mingw64\bin`

##

#### GLEW install

Download Binaries Windows 32-bit and 64-bit:

https://glew.sourceforge.net/index.html

Extract it somewhere

Copy bin Release x64

`C:\Users\<user>\Downloads\glew-2.1.0-win32\glew-2.1.0\bin\Release\x64`

to

`C:\mingw64\bin`

Copy lib Release x64

`C:\Users\<user>\Downloads\glew-2.1.0-win32\glew-2.1.0\lib\Release\x64`

to

`C:\mingw64\lib`

Copy include GL folder

`C:\Users\<user>\Downloads\glew-2.1.0-win32\glew-2.1.0\include`

to

`C:\mingw64\include`

##

#### GLFW install

Download 64-bit Windows binaries:

https://www.glfw.org/download.html

Extract it somewhere

Copy bin

`C:\Users\<user>\Downloads\glfw-3.3.8.bin.WIN64\lib-mingw-w64\glfw3.dll`

to

`C:\mingw64\bin`

Copy lib

`C:\Users\<user>\Downloads\glfw-3.3.8.bin.WIN64\lib-mingw-w64\libglfw3.a`

`C:\Users\<user>\Downloads\glfw-3.3.8.bin.WIN64\lib-mingw-w64\libglfw3dll.a`

to

`C:\mingw64\lib`

Copy include GLFW folder

`C:\Users\<user>\Downloads\glfw-3.3.8.bin.WIN64\include`

to

`C:\mingw64\include`

##

#### Compile server.cpp with g++:

`g++ server.cpp -lws2_32 -lGLEW32 -lglfw3 -lgdi32 -lopengl32 -o server`

##

#### Run the program:

`server.exe`
