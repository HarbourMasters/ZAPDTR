#pragma once

#include "ZResource.h"
#include "tinyxml2.h"

/**
 * @file ZText.h
 * @brief Handles text resource processing and management for game messages
 * 
 * This file defines the classes needed to handle text resources, including
 * individual message entries and the container class that manages them.
 */

/**
 * @brief Container for individual game message data
 * 
 * Stores all necessary information for a single message entry, including
 * its identifier, display properties, and the actual message content.
 */
class MessageEntry
{
public:
    /** @brief Unique identifier for the message (0-65535) */
    uint16_t id;
    
    /** @brief Determines the visual style of the textbox */
    uint8_t textboxType;
    
    /** @brief Vertical position of the textbox on screen (0-255) */
    uint8_t textboxYPos;
    
    /** @brief Memory segment where message data is stored */
    uint32_t segmentId;
    
    /** @brief Byte offset within segment to message data */
    uint32_t msgOffset;
    
    /** @brief Actual text content of the message */
    std::string msg;
};

/**
 * @brief Resource manager for game text and messages
 * 
 * Handles the loading, parsing, and management of text resources.
 * Inherits from ZResource to integrate with the resource management system.
 */
class ZText : public ZResource
{
public:
    /** @brief Collection of all messages in this text resource */
    std::vector<MessageEntry> messages;

    /**
     * @brief Initializes a new text resource
     * @param nParent Parent file containing this resource
     */
    ZText(ZFile* nParent);

    /**
     * @brief Processes raw binary data into structured message entries
     * Overrides ZResource::ParseRawData
     */
    void ParseRawData() override;

    /**
     * @brief Returns the resource type name for serialization
     * @return String identifier for this resource type
     */
    std::string GetSourceTypeName() const override;

    /**
     * @brief Returns the enumerated type of this resource
     * @return Resource type identifier
     */
    ZResourceType GetResourceType() const override;

    /**
     * @brief Calculates the total size of the raw resource data
     * @return Size in bytes of the raw data
     */
    size_t GetRawDataSize() const override;
};
