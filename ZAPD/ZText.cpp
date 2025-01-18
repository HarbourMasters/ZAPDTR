/*
 * @brief ZText.cpp - Handles parsing and processing of text/message data from ROM files
 * 
 * This file implements the ZText class which handles parsing and processing of 
 * text/message data from ROM files. It specifically handles message entries that
 * contain text box information, message content, and control codes.
 */

#include "ZText.h"

#include "Globals.h"
#include "Utils/BitConverter.h"
#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"

// Register this class as a file node type that can handle text resources
REGISTER_ZFILENODE(Text, ZText);

/*
 * @brief Constructor - Initializes a new ZText resource
 * @param nParent: Parent ZFile that contains this resource
 */
ZText::ZText(ZFile* nParent) : ZResource(nParent)
{
	RegisterRequiredAttribute("CodeOffset");
	RegisterOptionalAttribute("LangOffset", "0");
}

/*
 * @brief Parses raw text data from the ROM file
 * 
 * This method:
 * 1. Reads message entries from code segment
 * 2. Handles both regular and PAL language formats
 * 3. Processes control codes and message content
 */
void ZText::ParseRawData()
{
	ZResource::ParseRawData();

	const auto& rawData = parent->GetRawData();
	uint32_t currentPtr = StringHelper::StrToL(registeredAttributes.at("CodeOffset").value, 16);
	uint32_t langPtr = currentPtr;
	bool isPalLang = false;

	if (StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16) != 0)
	{
		langPtr = StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16);

		if (langPtr != currentPtr)
			isPalLang = true;
	}

	std::vector<uint8_t> codeData;

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		codeData = Globals::Instance->GetBaseromFile("code");
	else
		codeData = Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "code");

	// Process message entries until terminating ID is found (0xFFFC or 0xFFFF)
	while (true)
	{
		MessageEntry msgEntry;
		// Parse message header - ID and textbox properties
		msgEntry.id = BitConverter::ToInt16BE(codeData, currentPtr + 0);
		// Extract textbox type (high nibble) and Y position (low nibble)
		msgEntry.textboxType = (codeData[currentPtr + 2] & 0xF0) >> 4;
		msgEntry.textboxYPos = (codeData[currentPtr + 2] & 0x0F);

		// Handle PAL vs non-PAL language format differences
		if (isPalLang)
		{
			msgEntry.segmentId = (codeData[langPtr + 0]);
			msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 0) & 0x00FFFFFF;
		}
		else
		{
			msgEntry.segmentId = (codeData[langPtr + 4]);
			msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 4) & 0x00FFFFFF;
		}

		uint32_t msgPtr = msgEntry.msgOffset;

		unsigned char c = rawData[msgPtr];
		unsigned int extra = 0;
		bool stop = false;

		// Continue parsing until we are told to stop and all extra bytes are read
		while ((c != '\0' && !stop) || extra > 0)
		{
			msgEntry.msg += c;
			msgPtr++;

			// Some control codes require reading extra bytes
			if (extra == 0)
			{
				// End marker, so stop this message and do not read anything else
				if (c == 0x02)
				{
					stop = true;
				}
				else if (c == 0x05 || c == 0x13 || c == 0x0E || c == 0x0C || c == 0x1E || c == 0x06 ||
				    c == 0x14)
				{
					extra = 1;
				}
				// "Continue to new text ID", so stop this message and read two more bytes for the text ID
				else if (c == 0x07)
				{
					extra = 2;
					stop = true;
				}
				else if (c == 0x12 || c == 0x11)
				{
					extra = 2;
				}
				else if (c == 0x15)
				{
					extra = 3;
				}
			}
			else
			{
				extra--;
			}

			c = rawData[msgPtr];
		}

		messages.push_back(msgEntry);

		if (msgEntry.id == 0xFFFC || msgEntry.id == 0xFFFF)
			break;

		currentPtr += 8;

		if (isPalLang)
			langPtr += 4;
		else
			langPtr += 8;
	}

	int bp2 = 0;
}

/*
 * @brief Returns the source type name for this resource
 * @return The string "u8" indicating unsigned 8-bit data
 */
std::string ZText::GetSourceTypeName() const
{
	return "u8";
}

/*
 * @brief Returns the size of the raw data in bytes
 * @return Size of the resource data (1 byte)
 */
size_t ZText::GetRawDataSize() const
{
	return 1;
}

/*
 * @brief Returns the resource type identifier
 * @return ZResourceType::Text indicating this is a text resource
 */
ZResourceType ZText::GetResourceType() const
{
	return ZResourceType::Text;
}
