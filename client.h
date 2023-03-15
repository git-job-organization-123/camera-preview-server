struct Client {
  unsigned char* texture = nullptr; // Image screen
  short textureWidth;
  short textureHeight;
  bool free = true; // Free screen place
  std::thread* thread = nullptr;
  std::string connectingIP;
  std::string IP;
};
