#include "SetEntranceList.h"

#include "Globals.h"
#include "SetStartPositionList.h"
#include "Utils/BitConverter.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"
#include "ZRoom/ZRoom.h"

SetEntranceList::SetEntranceList(ZFile* nParent) : ZRoomCommand(nParent)
{
}

void SetEntranceList::DeclareReferences([[maybe_unused]] const std::string& prefix)
{
	if (segmentOffset != 0)
	{
		std::string varName =
			StringHelper::Sprintf("%sEntranceList0x%06X", prefix.c_str(), segmentOffset);
		parent->AddDeclarationPlaceholder(segmentOffset, varName);
	}
}

void SetEntranceList::ParseRawDataLate()
{
	// Parse Entrances and Generate Declaration
	uint32_t numEntrances = zRoom->parent->GetDeclarationSizeFromNeighbor(segmentOffset) / 2;
	uint32_t currentPtr = segmentOffset;

	entrances.reserve(numEntrances);
	for (uint32_t i = 0; i < numEntrances; i++)
	{
		Spawn entry(parent->GetRawData(), currentPtr);
		entrances.push_back(entry);

		currentPtr += 2;
	}
}

void SetEntranceList::DeclareReferencesLate([[maybe_unused]] const std::string& prefix)
{
	if (!entrances.empty())
	{
		std::string declaration;

		size_t index = 0;
		for (const auto& entry : entrances)
		{
			declaration += StringHelper::Sprintf("    { %s },", entry.GetBodySourceCode().c_str());
			if (index + 1 < entrances.size())
				declaration += "\n";

			index++;
		}

		std::string varName =
			StringHelper::Sprintf("%sEntranceList0x%06X", prefix.c_str(), segmentOffset);

		if (Globals::Instance->game != ZGame::MM_RETAIL)
			parent->AddDeclarationArray(segmentOffset, DeclarationAlignment::Align4,
			                            entrances.size() * 2, "Spawn", varName, entrances.size(),
			                            declaration);
		else
			parent->AddDeclarationArray(segmentOffset, DeclarationAlignment::Align4,
			                            entrances.size() * 2, "EntranceEntry", varName,
			                            entrances.size(), declaration);
	}
}

std::string SetEntranceList::GetBodySourceCode() const
{
	std::string listName;
	if (Globals::Instance->game != ZGame::MM_RETAIL)
		Globals::Instance->GetSegmentedPtrName(cmdArg2, parent, "Spawn", listName, parent->workerID);
	else
		Globals::Instance->GetSegmentedPtrName(cmdArg2, parent, "EntranceEntry", listName, parent->workerID);
	return StringHelper::Sprintf("SCENE_CMD_ENTRANCE_LIST(%s)", listName.c_str());
}

std::string SetEntranceList::GetCommandCName() const
{
	return "SCmdEntranceList";
}

RoomCommand SetEntranceList::GetRoomCommand() const
{
	return RoomCommand::SetEntranceList;
}

Spawn::Spawn(const std::vector<uint8_t>& rawData, uint32_t rawDataIndex)
{
	startPositionIndex = rawData.at(rawDataIndex + 0);
	roomToLoad = rawData.at(rawDataIndex + 1);
}

std::string Spawn::GetBodySourceCode() const
{
	return StringHelper::Sprintf("0x%02X, 0x%02X", startPositionIndex, roomToLoad);
}
