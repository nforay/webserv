#include "ContentTypeHeader.hpp"

ContentTypeHeader::ContentTypeHeader() {}
ContentTypeHeader::ContentTypeHeader(const ContentTypeHeader& other) {}
ContentTypeHeader::~ContentTypeHeader() {}
ContentTypeHeader& ContentTypeHeader::operator=(const ContentTypeHeader& other) { return *this; }

std::string ContentTypeHeader::getType() const { return "Content-Type"; }
Header* ContentTypeHeader::clone() const { return new ContentTypeHeader(); }