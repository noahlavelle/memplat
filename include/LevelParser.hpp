// LevelParser.h
#ifndef LEVELPARSER_H
#define LEVELPARSER_H

#include "ByteReader.hpp"
#include <cstddef>

class LevelParser {
  public:
    LevelParser(ByteReader reader);

    // Throws LevelParseError on malformed or truncated data.
    void loadScreen();

  private:
    ByteReader reader;
    int levelXCounter = 0;

    void loadObject(std::byte coordinate_byte, std::byte object_byte);
    void parseReservedRow(int type, int data);
    void parseSingleTile(int tile);
    void parseMetaTile(int metatile, int data);
    bool crossedScreenBoundary();
};

#endif // LEVELPARSER_H
