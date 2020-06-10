/*
**  GSC-18128-1, "Core Flight Executive Version 6.7"
**
**  Copyright (c) 2006-2019 United States Government as represented by
**  the Administrator of the National Aeronautics and Space Administration.
**  All Rights Reserved.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

/******************************************************************************
 * Message CCSDS Primary header implementations
 */
#include "cfe_msg_api.h"
#include "cfe_msg_priv.h"
#include "cfe_msg_defaults.h"
#include "cfe_error.h"

/* CCSDS Primary Standard definitions */
#define CFE_MSG_SIZE_OFFSET    7      /**< \brief CCSDS size offset */
#define CFE_MSG_CCSDSVER_MASK  0xE000 /**< \brief CCSDS version mask */
#define CFE_MSG_CCSDSVER_SHIFT 13     /**< \brief CCSDS version shift */
#define CFE_MSG_TYPE_MASK      0x1000 /**< \brief CCSDS type mask, command when set */
#define CFE_MSG_SHDR_MASK      0x0800 /**< \brief CCSDS secondary header mask, exists when set*/
#define CFE_MSG_APID_MASK      0x07FF /**< \brief CCSDS ApID mask */
#define CFE_MSG_SEGFLG_MASK    0xC000 /**< \brief CCSDS segmentation flag mask, all set = complete packet */
#define CFE_MSG_SEGFLG_CNT     0x0000 /**< \brief CCSDS Segment continuation flag */
#define CFE_MSG_SEGFLG_FIRST   0x4000 /**< \brief CCSDS Segment first flag */
#define CFE_MSG_SEGFLG_LAST    0x8000 /**< \brief CCSDS Segment last flag */
#define CFE_MSG_SEGFLG_UNSEG   0xC000 /**< \brief CCSDS Unsegmented flag */
#define CFE_MSG_SEQCNT_MASK    0x3FFF /**< \brief CCSDS Sequence count mask */

/******************************************************************************
 * CCSDS Primary header initialization - See header file for details
 */
void CFE_MSG_SetDefaultCCSDSPri(CFE_MSG_Message_t *MsgPtr)
{

    /* cFS standard is for secondary header to be present */
    CFE_MSG_SetHasSecondaryHeader(MsgPtr, true);

    /* cFS standard for CCSDS Version is Ver 1 = 0, Ver 2 = 1, but mission may redefine */
    CFE_MSG_SetHeaderVersion(MsgPtr, CFE_MISSION_CCSDSVER);

    /* Default bits of the APID, for whatever isn't set by MsgId */
    CFE_MSG_SetApId(MsgPtr, CFE_PLATFORM_DEFAULT_APID);

    /* Default to complete packets */
    CFE_MSG_SetSegmentationFlag(MsgPtr, CFE_MSG_SegFlag_Unsegmented);
}

/******************************************************************************
 * Get message header version - See API and header file for details
 */
int32 CFE_MSG_GetHeaderVersion(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_HeaderVersion_t *Version)
{

    if (MsgPtr == NULL || Version == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_GetHeaderField(MsgPtr->CCSDS.Pri.StreamId, Version, CFE_MSG_CCSDSVER_MASK);
    *Version >>= CFE_MSG_CCSDSVER_SHIFT;

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message header version - See API and header file for details
 */
int32 CFE_MSG_SetHeaderVersion(CFE_MSG_Message_t *MsgPtr, CFE_MSG_HeaderVersion_t Version)
{
    Version <<= CFE_MSG_CCSDSVER_SHIFT;
    if (MsgPtr == NULL || ((Version & CFE_MSG_CCSDSVER_MASK) != 0))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_SetHeaderField(MsgPtr->CCSDS.Pri.StreamId, Version, CFE_MSG_CCSDSVER_MASK);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Get message type - See API and header file for details
 */
int32 CFE_MSG_GetType(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_Type_t *Type)
{

    if (MsgPtr == NULL || Type == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    if (MsgPtr->CCSDS.Pri.StreamId[0] & (CFE_MSG_TYPE_MASK >> 8))
    {
        *Type = CFE_MSG_Type_Cmd;
    }
    else
    {
        *Type = CFE_MSG_Type_Tlm;
    }

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message type - See API and header file for details
 */
int32 CFE_MSG_SetType(CFE_MSG_Message_t *MsgPtr, CFE_MSG_Type_t Type)
{
    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    if (Type == CFE_MSG_Type_Cmd)
    {
        MsgPtr->CCSDS.Pri.StreamId[0] |= CFE_MSG_TYPE_MASK >> 8;
    }
    else
    {
        MsgPtr->CCSDS.Pri.StreamId[0] &= ~(CFE_MSG_TYPE_MASK >> 8);
    }

    return CFE_SUCCESS;
}

/******************************************************************************
 * Get message has secondary header flag - See API and header file for details
 */
int32 CFE_MSG_GetHasSecondaryHeader(const CFE_MSG_Message_t *MsgPtr, bool *HasSecondary)
{

    if (MsgPtr == NULL || HasSecondary == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    *HasSecondary = (MsgPtr->CCSDS.Pri.StreamId[0] & (CFE_MSG_SHDR_MASK >> 8)) != 0;

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message has secondary header flag - See API and header file for details
 */
int32 CFE_MSG_SetHasSecondaryHeader(CFE_MSG_Message_t *MsgPtr, bool HasSecondary)
{
    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    if (HasSecondary)
    {
        MsgPtr->CCSDS.Pri.StreamId[0] |= CFE_MSG_SHDR_MASK >> 8;
    }
    else
    {
        MsgPtr->CCSDS.Pri.StreamId[0] &= ~(CFE_MSG_SHDR_MASK >> 8);
    }

    return CFE_SUCCESS;
}

/******************************************************************************
 * Get message application ID - See API and header file for details
 */
int32 CFE_MSG_GetApId(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_ApId_t *ApId)
{

    if (MsgPtr == NULL || ApId == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_GetHeaderField(MsgPtr->CCSDS.Pri.StreamId, ApId, CFE_MSG_APID_MASK);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message application ID - See API and header file for details
 */
int32 CFE_MSG_SetApId(CFE_MSG_Message_t *MsgPtr, CFE_MSG_ApId_t ApId)
{
    if (MsgPtr == NULL || ((ApId & CFE_MSG_APID_MASK) != 0))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_SetHeaderField(MsgPtr->CCSDS.Pri.StreamId, ApId, CFE_MSG_APID_MASK);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Get message segmentation flag - See API and header file for details
 */
int32 CFE_MSG_GetSegmentationFlag(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_SegmentationFlag_t *SegFlag)
{

    uint16 rawval;

    if (MsgPtr == NULL || SegFlag == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_GetHeaderField(MsgPtr->CCSDS.Pri.Sequence, &rawval, CFE_MSG_SEGFLG_MASK);

    switch (rawval)
    {
        case CFE_MSG_SEGFLG_CNT:
            *SegFlag = CFE_MSG_SegFlag_Continue;
            break;
        case CFE_MSG_SEGFLG_FIRST:
            *SegFlag = CFE_MSG_SegFlag_First;
            break;
        case CFE_MSG_SEGFLG_LAST:
            *SegFlag = CFE_MSG_SegFlag_Last;
            break;
        case CFE_MSG_SEGFLG_UNSEG:
        default:
            *SegFlag = CFE_MSG_SegFlag_Unsegmented;
    }

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message segmentation flag - See API and header file for details
 */
int32 CFE_MSG_SetSegmentationFlag(CFE_MSG_Message_t *MsgPtr, CFE_MSG_SegmentationFlag_t SegFlag)
{
    uint16 rawval = 0;
    int32  status = CFE_SUCCESS;

    if (MsgPtr == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    switch (SegFlag)
    {
        case CFE_MSG_SegFlag_Continue:
            rawval = CFE_MSG_SEGFLG_CNT;
            break;
        case CFE_MSG_SegFlag_First:
            rawval = CFE_MSG_SEGFLG_FIRST;
            break;
        case CFE_MSG_SegFlag_Last:
            rawval = CFE_MSG_SEGFLG_LAST;
            break;
        case CFE_MSG_SegFlag_Unsegmented:
            rawval = CFE_MSG_SEGFLG_UNSEG;
            break;
        case CFE_MSG_SegFlag_Invalid:
        default:
            status = CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_SetHeaderField(MsgPtr->CCSDS.Pri.Sequence, rawval, CFE_MSG_SEGFLG_MASK);

    return status;
}

/******************************************************************************
 * Get message sequence count - See API and header file for details
 */
int32 CFE_MSG_GetSequenceCount(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_SequenceCount_t *SeqCnt)
{

    if (MsgPtr == NULL || SeqCnt == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_GetHeaderField(MsgPtr->CCSDS.Pri.Sequence, SeqCnt, CFE_MSG_SEQCNT_MASK);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message sequence count - See API and header file for details
 */
int32 CFE_MSG_SetSequenceCount(CFE_MSG_Message_t *MsgPtr, CFE_MSG_SequenceCount_t SeqCnt)
{
    if (MsgPtr == NULL || ((SeqCnt & CFE_MSG_SEQCNT_MASK) != 0))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    CFE_MSG_SetHeaderField(MsgPtr->CCSDS.Pri.Sequence, SeqCnt, CFE_MSG_SEQCNT_MASK);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Get message size - See API and header file for details
 */
int32 CFE_MSG_GetSize(const CFE_MSG_Message_t *MsgPtr, CFE_MSG_Size_t *Size)
{

    if (MsgPtr == NULL || Size == NULL)
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    *Size = (MsgPtr->CCSDS.Pri.Length[0] << 8) + MsgPtr->CCSDS.Pri.Length[1] + CFE_MSG_SIZE_OFFSET;

    return CFE_SUCCESS;
}

/******************************************************************************
 * Set message size - See API and header file for details
 */
int32 CFE_MSG_SetSize(CFE_MSG_Message_t *MsgPtr, CFE_MSG_Size_t Size)
{
    if (MsgPtr == NULL || Size < CFE_MSG_SIZE_OFFSET || Size > (0xFFFF + CFE_MSG_SIZE_OFFSET))
    {
        return CFE_MSG_BAD_ARGUMENT;
    }

    /* Size is CCSDS header is total packet size - CFE_MSG_SIZE_OFFSET (7) */
    Size -= CFE_MSG_SIZE_OFFSET;

    MsgPtr->CCSDS.Pri.Length[0] = (Size >> 8) & 0xFF;
    MsgPtr->CCSDS.Pri.Length[1] = Size & 0xFF;

    return CFE_SUCCESS;
}