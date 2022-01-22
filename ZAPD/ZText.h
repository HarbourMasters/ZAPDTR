#pragma once

#include "ZResource.h"
#include "tinyxml2.h"

class MessageEntry
{
public:
	uint16_t id;
	uint8_t textboxType;
	uint8_t textboxYPos;
	uint32_t segmentId;
	uint32_t msgOffset;
	std::string msg;
};

class ZText : public ZResource
{
public:
	std::vector<MessageEntry> messages;

	ZText(ZFile* nParent);

	//void ParseXML(tinyxml2::XMLElement* reader) override;
	void ParseRawData() override;
	//void DeclareReferences(const std::string& prefix) override;

	//Declaration* DeclareVar(const std::string& prefix, const std::string& bodyStr) override;
	//std::string GetBodySourceCode() const override;
	//void CalcHash() override;
	//void Save(const fs::path& outFolder) override;

	//bool IsExternalResource() const override;
	std::string GetSourceTypeName() const override;
	ZResourceType GetResourceType() const override;
	//std::string GetExternalExtension() const override;

	size_t GetRawDataSize() const override;
	//std::string GetDefaultName(const std::string& prefix) const override;
};
