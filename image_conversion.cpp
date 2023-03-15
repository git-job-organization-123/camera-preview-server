// Convert YUV_420_888 image to RGB
// Bug: fails to convert green color
void YUV420888_to_RGB(const char* y, const char* u, const char* v, unsigned char* rgb, int width, int height, int y_pixel_stride, int y_row_stride, int uv_pixel_stride, int uv_row_stride) {
  for (int y_coord = 0; y_coord < height; y_coord++) {
    int y_idx = y_coord * y_row_stride;
    int uv_idx = (y_coord >> 1) * uv_row_stride;
    for (int x_coord = 0; x_coord < width; x_coord++) {
      // Extract the Y, U, and V values
      int Y = y[y_idx + x_coord * y_pixel_stride] & 0xff;
      int U = u[uv_idx + (x_coord >> 1) * uv_pixel_stride] & 0xff;
      int V = v[uv_idx + (x_coord >> 1) * uv_pixel_stride + 1] & 0xff;

      int r = Y + (1.370705 * (V - 128));
      int g = Y - (0.698001 * (V - 128)) - (0.337633 * (U - 128));
      int b = Y + (1.732446 * (U - 128));

      // Clamp the R, G, and B values to the range [0, 255]
      r = r < 0 ? 0 : (r > 255 ? 255 : r);
      g = g < 0 ? 0 : (g > 255 ? 255 : g);
      b = b < 0 ? 0 : (b > 255 ? 255 : b);

      // Compute the index for the R, G, and B channels
      int rgb_idx = (y_coord * width + x_coord) * 3;

      // Set the R, G, and B values
      rgb[rgb_idx] = (unsigned char)g;
      rgb[rgb_idx + 1] = (unsigned char)r;
      rgb[rgb_idx + 2] = (unsigned char)b;
    }
  }
}
