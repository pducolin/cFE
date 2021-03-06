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
** File: cfe_sb_task.c
**
** Purpose:
**      This file contains the source code for the SB task.
**
** Author:   R.McGraw/SSI
**
******************************************************************************/

/* Include Files */

#include "cfe_sb.h"
#include "cfe_sb_events.h"
#include "cfe_evs.h"
#include "cfe_sb_priv.h"
#include "osapi.h"
#include "cfe_version.h"
#include "cfe_msgids.h"
#include "cfe_error.h"
#include "cfe_es.h"
#include "cfe_psp.h"
#include "cfe_es_msg.h"
#include "cfe_sb_verify.h"
#include <string.h>

/*  Task Globals */
CFE_SB_Global_t                CFE_SB_Global;

/* Local structure for file writing callbacks */
typedef struct
{
    const char *Filename;   /* File name for error reporting */
    osal_id_t   Fd;         /* File id for writing */
    uint32      FileSize;   /* File size for reporting */
    uint32      EntryCount; /* Entry count for reporting */
    int32       Status;     /* File write status */
} CFE_SB_FileWriteCallback_t;

/******************************************************************************
**  Function:  CFE_SB_TaskMain()
**
**  Purpose:
**    Main loop for Software Bus task, used to process SB commands.
**
**  Arguments:
**    none
**
**  Return:
**    none
*/
void CFE_SB_TaskMain(void)
{
    int32            Status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(CFE_MISSION_SB_MAIN_PERF_ID);

    Status = CFE_SB_AppInit();
    
    if(Status != CFE_SUCCESS)
    {
      CFE_ES_WriteToSysLog("SB:Application Init Failed,RC=0x%08X\n", (unsigned int)Status);
      CFE_ES_PerfLogExit(CFE_MISSION_SB_MAIN_PERF_ID);
      /* Note: CFE_ES_ExitApp will not return */
      CFE_ES_ExitApp(CFE_ES_RunStatus_CORE_APP_INIT_ERROR);
    }/* end if */

    /*
     * Wait for other apps to start.
     * It is important that the core apps are present before this starts receiving
     * messages from the command pipe, as some of those handlers might depend on
     * the other core apps.
     */
    CFE_ES_WaitForSystemState(CFE_ES_SystemState_CORE_READY, CFE_PLATFORM_CORE_MAX_STARTUP_MSEC);

    /* Main loop */
    while (Status == CFE_SUCCESS)
    {    
        /* Increment the Main task Execution Counter */
        CFE_ES_IncrementTaskCounter();

        CFE_ES_PerfLogExit(CFE_MISSION_SB_MAIN_PERF_ID);

        /* Pend on receipt of packet */
        Status = CFE_SB_ReceiveBuffer(&SBBufPtr,
                                CFE_SB_Global.CmdPipe,
                                CFE_SB_PEND_FOREVER);

        CFE_ES_PerfLogEntry(CFE_MISSION_SB_MAIN_PERF_ID);

        if(Status == CFE_SUCCESS)
        {
            /* Process cmd pipe msg */
            CFE_SB_ProcessCmdPipePkt(SBBufPtr);
        }else{
            CFE_ES_WriteToSysLog("SB:Error reading cmd pipe,RC=0x%08X\n",(unsigned int)Status);
        }/* end if */

    }/* end while */

    /* while loop exits only if CFE_SB_ReceiveBuffer returns error */
    CFE_ES_ExitApp(CFE_ES_RunStatus_CORE_APP_RUNTIME_ERROR);

}/* end CFE_SB_TaskMain */



/******************************************************************************
**  Function:  CFE_SB_AppInit()
**
**  Purpose:
**    Initialization routine for SB application. This routine is executed when
**    the SB application is started by Executive Services.
**
**  Arguments:
**    none
**
**  Return:
**    CFE_SUCCESS if no errors, otherwise this function returns the error code
**    that was received from the function that detected the error.
**    
*/
int32 CFE_SB_AppInit(void){

    uint32 CfgFileEventsToFilter = 0;    
    CFE_ES_MemPoolBuf_t TmpPtr;
    int32  Status;
    
    Status = CFE_ES_RegisterApp();

    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Call to CFE_ES_RegisterApp Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */

    /* Get the assigned Application ID for the SB Task */
    CFE_ES_GetAppID(&CFE_SB_Global.AppId);

    /* Process the platform cfg file events to be filtered */
    if(CFE_PLATFORM_SB_FILTERED_EVENT1 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT1;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK1;      
      CfgFileEventsToFilter++;
    }/* end if */           

    if(CFE_PLATFORM_SB_FILTERED_EVENT2 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT2;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK2;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT3 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT3;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK3;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT4 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT4;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK4;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT5 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT5;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK5;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT6 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT6;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK6;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT7 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT7;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK7;      
      CfgFileEventsToFilter++;
    }/* end if */      

    if(CFE_PLATFORM_SB_FILTERED_EVENT8 != 0){
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].EventID = CFE_PLATFORM_SB_FILTERED_EVENT8;
      CFE_SB_Global.EventFilters[CfgFileEventsToFilter].Mask    = CFE_PLATFORM_SB_FILTER_MASK8;      
      CfgFileEventsToFilter++;
    }/* end if */
    
    /* Be sure the number of events to register for filtering   
    ** does not exceed CFE_PLATFORM_EVS_MAX_EVENT_FILTERS */
    if(CFE_PLATFORM_EVS_MAX_EVENT_FILTERS < CfgFileEventsToFilter){
      CfgFileEventsToFilter = CFE_PLATFORM_EVS_MAX_EVENT_FILTERS;
    }/* end if */


    /* Register event filter table... */
    Status = CFE_EVS_Register(CFE_SB_Global.EventFilters,
                              CfgFileEventsToFilter,
                              CFE_EVS_EventFilter_BINARY);
    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Call to CFE_EVS_Register Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */
    
    CFE_ES_WriteToSysLog("SB:Registered %d events for filtering\n",(int)CfgFileEventsToFilter);

    CFE_MSG_Init(&CFE_SB_Global.HKTlmMsg.Hdr.Msg,
                 CFE_SB_ValueToMsgId(CFE_SB_HK_TLM_MID),
                 sizeof(CFE_SB_Global.HKTlmMsg));

    CFE_MSG_Init(&CFE_SB_Global.PrevSubMsg.Hdr.Msg,
                 CFE_SB_ValueToMsgId(CFE_SB_ALLSUBS_TLM_MID),
                 sizeof(CFE_SB_Global.PrevSubMsg));

    /* Populate the fixed fields in the HK Tlm Msg */
    CFE_SB_Global.HKTlmMsg.Payload.MemPoolHandle = CFE_SB_Global.Mem.PoolHdl;
    
    /* Populate the fixed fields in the Stat Tlm Msg */
    CFE_SB_Global.StatTlmMsg.Payload.MaxMsgIdsAllowed = CFE_PLATFORM_SB_MAX_MSG_IDS;
    CFE_SB_Global.StatTlmMsg.Payload.MaxPipesAllowed  = CFE_PLATFORM_SB_MAX_PIPES;
    CFE_SB_Global.StatTlmMsg.Payload.MaxMemAllowed    = CFE_PLATFORM_SB_BUF_MEMORY_BYTES;
    CFE_SB_Global.StatTlmMsg.Payload.MaxPipeDepthAllowed = OS_QUEUE_MAX_DEPTH;
    CFE_SB_Global.StatTlmMsg.Payload.MaxSubscriptionsAllowed =
                    ((CFE_PLATFORM_SB_MAX_MSG_IDS)*(CFE_PLATFORM_SB_MAX_DEST_PER_PKT));
    
    Status = CFE_SB_CreatePipe(&CFE_SB_Global.CmdPipe,
                                CFE_SB_CMD_PIPE_DEPTH,
                                CFE_SB_CMD_PIPE_NAME);
    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Call to CFE_SB_CreatePipe Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */                                

    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_SB_CMD_MID),CFE_SB_Global.CmdPipe);

    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Subscribe to Cmds Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */
        
    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_SB_SEND_HK_MID),CFE_SB_Global.CmdPipe);

    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Subscribe to HK Request Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */

    Status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CFE_SB_SUB_RPT_CTRL_MID),CFE_SB_Global.CmdPipe);

    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Subscribe to Subscription Report Request Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */
     
    /* Ensure a ground commanded reset does not get blocked if SB mem pool  */
    /* becomes fully configured (DCR6772) */
    Status = CFE_ES_GetPoolBuf(&TmpPtr, CFE_SB_Global.Mem.PoolHdl,
                                        sizeof(CFE_ES_RestartCmd_t));

    if(Status < 0){
      CFE_ES_WriteToSysLog("SB:Init error, GetPool Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */

    /* Return mem block used on previous call,the actual memory is not needed.*/
    /* The SB mem pool is now configured with a block size for the reset cmd. */
    Status = CFE_ES_PutPoolBuf(CFE_SB_Global.Mem.PoolHdl, TmpPtr);

    if(Status < 0){
      CFE_ES_WriteToSysLog("SB:Init error, PutPool Failed:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */    
    
    Status = CFE_EVS_SendEvent(CFE_SB_INIT_EID,
                              CFE_EVS_EventType_INFORMATION,
                              "cFE SB Initialized");
    if(Status != CFE_SUCCESS){
      CFE_ES_WriteToSysLog("SB:Error sending init event:RC=0x%08X\n",(unsigned int)Status);
      return Status;
    }/* end if */
                      
    return CFE_SUCCESS;                      

}/* end CFE_SB_AppInit */


/******************************************************************************
**  Function:  CFE_SB_VerifyCmdLength()
**
**  Purpose:
**    Function to verify the length of incoming SB command packets
**
**  Arguments:
**    Message pointer and expected length
**
**  Return:
**    true if length is acceptable
*/
bool CFE_SB_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    CFE_MSG_Size_t    ActualLength = 0;
    CFE_MSG_FcnCode_t FcnCode      = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(CFE_SB_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode,
                          (unsigned int)ActualLength, (unsigned int)ExpectedLength);
        result = false;
        ++CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter;
    }

    return(result);

} /* End of CFE_SB_VerifyCmdLength() */



/******************************************************************************
**  Function:  CFE_SB_ProcessCmdPipePkt()
**
**  Purpose:
**    Function to control actions when an SB command is received.
**
**  Arguments:
**    Software bus buffer pointer
**
**  Return:
**    none
*/
void CFE_SB_ProcessCmdPipePkt(CFE_SB_Buffer_t *SBBufPtr)
{
   CFE_SB_MsgId_t MessageID = CFE_SB_INVALID_MSG_ID;
   CFE_MSG_FcnCode_t FcnCode = 0;

   CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MessageID);

   switch(CFE_SB_MsgIdToValue(MessageID)){

      case CFE_SB_SEND_HK_MID:
         /* Note: Command counter not incremented for this command */
         CFE_SB_SendHKTlmCmd((CFE_MSG_CommandHeader_t *)SBBufPtr);
         break;

      case CFE_SB_SUB_RPT_CTRL_MID:
         /* Note: Command counter not incremented for this command */
         CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);
         switch (FcnCode) {
            case CFE_SB_SEND_PREV_SUBS_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_SendPrevSubsCmd_t)))
                {
                    CFE_SB_SendPrevSubsCmd((CFE_SB_SendPrevSubsCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_ENABLE_SUB_REPORTING_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_EnableSubReportingCmd_t)))
                {
                    CFE_SB_EnableSubReportingCmd((CFE_SB_EnableSubReportingCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_DISABLE_SUB_REPORTING_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_DisableSubReportingCmd_t)))
                {
                    CFE_SB_DisableSubReportingCmd((CFE_SB_DisableSubReportingCmd_t *)SBBufPtr);
                }
                break;

            default:
               CFE_EVS_SendEvent(CFE_SB_BAD_CMD_CODE_EID,CFE_EVS_EventType_ERROR,
                     "Invalid Cmd, Unexpected Command Code %u", (unsigned int)FcnCode);
               CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
               break;
         } /* end switch on cmd code */
         break;

      case CFE_SB_CMD_MID:
         CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &FcnCode);
         switch (FcnCode) {
            case CFE_SB_NOOP_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_NoopCmd_t)))
                {
                    CFE_SB_NoopCmd((CFE_SB_NoopCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_RESET_COUNTERS_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_ResetCountersCmd_t)))
                {
                    /* Note: Command counter not incremented for this command */
                    CFE_SB_ResetCountersCmd((CFE_SB_ResetCountersCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_SEND_SB_STATS_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_SendSbStatsCmd_t)))
                {
                    CFE_SB_SendStatsCmd((CFE_SB_SendSbStatsCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_WRITE_ROUTING_INFO_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_WriteRoutingInfoCmd_t)))
                {
                    CFE_SB_WriteRoutingInfoCmd((CFE_SB_WriteRoutingInfoCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_ENABLE_ROUTE_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_EnableRouteCmd_t)))
                {
                    CFE_SB_EnableRouteCmd((CFE_SB_EnableRouteCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_DISABLE_ROUTE_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_DisableRouteCmd_t)))
                {
                    CFE_SB_DisableRouteCmd((CFE_SB_DisableRouteCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_WRITE_PIPE_INFO_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_WritePipeInfoCmd_t)))
                {
                    CFE_SB_WritePipeInfoCmd((CFE_SB_WritePipeInfoCmd_t *)SBBufPtr);
                }
                break;

            case CFE_SB_WRITE_MAP_INFO_CC:
                if (CFE_SB_VerifyCmdLength(&SBBufPtr->Msg, sizeof(CFE_SB_WriteMapInfoCmd_t)))
                {
                    CFE_SB_WriteMapInfoCmd((CFE_SB_WriteMapInfoCmd_t *)SBBufPtr);
                }
                break;

            default:
               CFE_EVS_SendEvent(CFE_SB_BAD_CMD_CODE_EID,CFE_EVS_EventType_ERROR,
                     "Invalid Cmd, Unexpected Command Code %u", FcnCode);
               CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
               break;
         } /* end switch on cmd code */
         break;

         default:
            CFE_EVS_SendEvent(CFE_SB_BAD_MSGID_EID,CFE_EVS_EventType_ERROR,
                  "Invalid Cmd, Unexpected Msg Id: 0x%x",
                  (unsigned int)CFE_SB_MsgIdToValue(MessageID));
            CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
            break;

   } /* end switch on MsgId */

} /* end CFE_SB_ProcessCmdPipePkt */



/******************************************************************************
**  Function:  CFE_SB_NoopCmd()
**
**  Purpose:
**    Handler function the SB command
**
*/
int32 CFE_SB_NoopCmd(const CFE_SB_NoopCmd_t *data)
{
    CFE_EVS_SendEvent(CFE_SB_CMD0_RCVD_EID,CFE_EVS_EventType_INFORMATION,
            "No-op Cmd Rcvd. %s", CFE_VERSION_STRING);
    CFE_SB_Global.HKTlmMsg.Payload.CommandCounter++;

    return CFE_SUCCESS;
}

/******************************************************************************
**  Function:  CFE_SB_ResetCountersCmd()
**
**  Purpose:
**    Handler function the SB command
**
*/
int32 CFE_SB_ResetCountersCmd(const CFE_SB_ResetCountersCmd_t *data)
{
    CFE_EVS_SendEvent(CFE_SB_CMD1_RCVD_EID,CFE_EVS_EventType_DEBUG,
            "Reset Counters Cmd Rcvd");

    CFE_SB_ResetCounters();

    return CFE_SUCCESS;
}

/******************************************************************************
**  Function:  CFE_SB_EnableSubReportingCmd()
**
**  Purpose:
**    Handler function the SB command
**
*/
int32 CFE_SB_EnableSubReportingCmd(const CFE_SB_EnableSubReportingCmd_t *data)
{
    CFE_SB_SetSubscriptionReporting(CFE_SB_ENABLE);
    return CFE_SUCCESS;
}

/******************************************************************************
**  Function:  CFE_SB_DisableSubReportingCmd()
**
**  Purpose:
**    Handler function the SB command
**
*/
int32 CFE_SB_DisableSubReportingCmd(const CFE_SB_DisableSubReportingCmd_t *data)
{
    CFE_SB_SetSubscriptionReporting(CFE_SB_DISABLE);
    return CFE_SUCCESS;
}


/******************************************************************************
**  Function:  CFE_SB_SendHKTlmCmd()
**
**  Purpose:
**    Function to send the SB housekeeping packet.
**
**  Arguments:
**    none
**
**  Notes:
**    Command counter not incremented for this command
**
**  Return:
**    none
*/
int32 CFE_SB_SendHKTlmCmd(const CFE_MSG_CommandHeader_t *data)
{
    CFE_SB_LockSharedData(__FILE__, __LINE__);

    CFE_SB_Global.HKTlmMsg.Payload.MemInUse        = CFE_SB_Global.StatTlmMsg.Payload.MemInUse;
    CFE_SB_Global.HKTlmMsg.Payload.UnmarkedMem     = CFE_PLATFORM_SB_BUF_MEMORY_BYTES - CFE_SB_Global.StatTlmMsg.Payload.PeakMemInUse;

    CFE_SB_UnlockSharedData(__FILE__, __LINE__);

    CFE_SB_TimeStampMsg(&CFE_SB_Global.HKTlmMsg.Hdr.Msg);
    CFE_SB_TransmitMsg(&CFE_SB_Global.HKTlmMsg.Hdr.Msg, true);

    return CFE_SUCCESS;
}/* end CFE_SB_SendHKTlmCmd */


/******************************************************************************
**  Function:  CFE_SB_ResetCounters()
**
**  Purpose:
**    Function to reset the SB housekeeping counters.
**
**  Arguments:
**    none
**
**  Notes:
**    Command counter not incremented for this command
**
**  Return:
**    none
*/
void CFE_SB_ResetCounters(void){

   CFE_SB_Global.HKTlmMsg.Payload.CommandCounter          = 0;
   CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter           = 0;
   CFE_SB_Global.HKTlmMsg.Payload.NoSubscribersCounter    = 0;
   CFE_SB_Global.HKTlmMsg.Payload.DuplicateSubscriptionsCounter = 0;
   CFE_SB_Global.HKTlmMsg.Payload.MsgSendErrorCounter       = 0;
   CFE_SB_Global.HKTlmMsg.Payload.MsgReceiveErrorCounter    = 0;
   CFE_SB_Global.HKTlmMsg.Payload.InternalErrorCounter      = 0;
   CFE_SB_Global.HKTlmMsg.Payload.CreatePipeErrorCounter    = 0;
   CFE_SB_Global.HKTlmMsg.Payload.SubscribeErrorCounter     = 0;
   CFE_SB_Global.HKTlmMsg.Payload.PipeOverflowErrorCounter  = 0;
   CFE_SB_Global.HKTlmMsg.Payload.MsgLimitErrorCounter        = 0;

}/* end CFE_SB_ResetCounters */


/******************************************************************************
**  Function:  CFE_SB_EnableRouteCmd()
**
**  Purpose:
**    SB internal function to enable a specific route. A route is defined as a
**    MsgId/PipeId combination.
**
**  Arguments:
**    MsgPtr  : pointer to the message
**
**  Return:
**    None
*/
int32 CFE_SB_EnableRouteCmd(const CFE_SB_EnableRouteCmd_t *data)
{
    CFE_SB_MsgId_t          MsgId;
    CFE_SB_PipeD_t          *PipeDscPtr;
    CFE_SB_DestinationD_t   *DestPtr;
    const CFE_SB_RouteCmd_Payload_t      *CmdPtr;
    uint16 PendingEventID;

    PendingEventID = 0;
    CmdPtr = &data->Payload;

    MsgId  = CmdPtr->MsgId;

    CFE_SB_LockSharedData(__func__,__LINE__);

    /* check cmd parameters */
    PipeDscPtr = CFE_SB_LocatePipeDescByID(CmdPtr->Pipe);
    if(!CFE_SB_IsValidMsgId(MsgId) ||
       !CFE_SB_PipeDescIsMatch(PipeDscPtr,CmdPtr->Pipe))
    {
        PendingEventID = CFE_SB_ENBL_RTE3_EID;
        CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
    }
    else
    {
        DestPtr = CFE_SB_GetDestPtr(CFE_SBR_GetRouteId(MsgId), CmdPtr->Pipe);
        if(DestPtr == NULL)
        {
            PendingEventID = CFE_SB_ENBL_RTE1_EID;
            CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
        }
        else
        {
            DestPtr->Active = CFE_SB_ACTIVE;
            PendingEventID = CFE_SB_ENBL_RTE2_EID;
            CFE_SB_Global.HKTlmMsg.Payload.CommandCounter++;
        }

    }/* end if */    

    CFE_SB_UnlockSharedData(__func__,__LINE__);

    switch(PendingEventID)
    {
        case CFE_SB_ENBL_RTE1_EID:
            CFE_EVS_SendEvent(CFE_SB_ENBL_RTE1_EID,CFE_EVS_EventType_ERROR,
                "Enbl Route Cmd:Route does not exist.Msg 0x%x,Pipe %lu",
                (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
        case CFE_SB_ENBL_RTE3_EID:
            CFE_EVS_SendEvent(CFE_SB_ENBL_RTE3_EID,CFE_EVS_EventType_ERROR,
                      "Enbl Route Cmd:Invalid Param.Msg 0x%x,Pipe %lu",
                      (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                      CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
        case CFE_SB_ENBL_RTE2_EID:
            CFE_EVS_SendEvent(CFE_SB_ENBL_RTE2_EID,CFE_EVS_EventType_DEBUG,
                      "Enabling Route,Msg 0x%x,Pipe %lu",
                      (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                      CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
    }

    return CFE_SUCCESS;
}/* end CFE_SB_EnableRouteCmd */



/******************************************************************************
**  Function:  CFE_SB_DisableRouteCmd()
**
**  Purpose:
**    SB internal function to disable a specific route. A route is defined as a
**    MsgId/PipeId combination.
**
**  Arguments:
**    MsgPtr  : pointer to the message
**
**  Return:
**    None
*/
int32 CFE_SB_DisableRouteCmd(const CFE_SB_DisableRouteCmd_t *data)
{
    CFE_SB_MsgId_t          MsgId;
    CFE_SB_PipeD_t          *PipeDscPtr;
    CFE_SB_DestinationD_t   *DestPtr;
    const CFE_SB_RouteCmd_Payload_t      *CmdPtr;
    uint16 PendingEventID;

    PendingEventID = 0;
    CmdPtr = &data->Payload;

    MsgId  = CmdPtr->MsgId;

    CFE_SB_LockSharedData(__func__,__LINE__);

    /* check cmd parameters */
    PipeDscPtr = CFE_SB_LocatePipeDescByID(CmdPtr->Pipe);
    if(!CFE_SB_IsValidMsgId(MsgId) ||
       !CFE_SB_PipeDescIsMatch(PipeDscPtr,CmdPtr->Pipe))
    {
        PendingEventID = CFE_SB_DSBL_RTE3_EID;
        CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
    }
    else
    {
        DestPtr = CFE_SB_GetDestPtr(CFE_SBR_GetRouteId(MsgId), CmdPtr->Pipe);
        if(DestPtr == NULL)
        {
            PendingEventID = CFE_SB_DSBL_RTE1_EID;
            CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
        }
        else
        {
            DestPtr->Active = CFE_SB_INACTIVE;
            PendingEventID = CFE_SB_DSBL_RTE2_EID;
            CFE_SB_Global.HKTlmMsg.Payload.CommandCounter++;
        }

    }/* end if */    

    CFE_SB_UnlockSharedData(__func__,__LINE__);


    switch(PendingEventID)
    {
        case CFE_SB_DSBL_RTE1_EID:
            CFE_EVS_SendEvent(CFE_SB_DSBL_RTE1_EID,CFE_EVS_EventType_ERROR,
                "Disable Route Cmd:Route does not exist,Msg 0x%x,Pipe %lu",
                (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
        case CFE_SB_DSBL_RTE3_EID:
            CFE_EVS_SendEvent(CFE_SB_DSBL_RTE3_EID,CFE_EVS_EventType_ERROR,
                   "Disable Route Cmd:Invalid Param.Msg 0x%x,Pipe %lu",
                   (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                   CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
        case CFE_SB_DSBL_RTE2_EID:
            CFE_EVS_SendEvent(CFE_SB_DSBL_RTE2_EID,CFE_EVS_EventType_DEBUG,
                      "Route Disabled,Msg 0x%x,Pipe %lu",
                      (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                      CFE_RESOURCEID_TO_ULONG(CmdPtr->Pipe));
            break;
    }

    return CFE_SUCCESS;
}/* end CFE_SB_DisableRouteCmd */


/******************************************************************************
**  Function:  CFE_SB_SendStatsCmd()
**
**  Purpose:
**    SB internal function to send a Software Bus statistics packet
**
**  Arguments:
**    None
**
**  Return:
**    None
*/
int32 CFE_SB_SendStatsCmd(const CFE_SB_SendSbStatsCmd_t *data)
{
    uint32 PipeDscCount;
    uint32 PipeStatCount;
    CFE_SB_PipeD_t *PipeDscPtr;
    CFE_SB_PipeDepthStats_t *PipeStatPtr;

    CFE_SB_LockSharedData(__FILE__, __LINE__);

    /* Collect data on pipes */
    PipeDscCount = CFE_PLATFORM_SB_MAX_PIPES;
    PipeStatCount = CFE_MISSION_SB_MAX_PIPES;
    PipeDscPtr = CFE_SB_Global.PipeTbl;
    PipeStatPtr = CFE_SB_Global.StatTlmMsg.Payload.PipeDepthStats;

    while (PipeDscCount > 0 && PipeStatCount > 0)
    {
        if (CFE_SB_PipeDescIsUsed(PipeDscPtr))
        {
            PipeStatPtr->PipeId = PipeDscPtr->PipeId;
            PipeStatPtr->InUse = PipeDscPtr->CurrentDepth;
            PipeStatPtr->PeakInUse = PipeDscPtr->PeakDepth;
            PipeStatPtr->Depth = PipeDscPtr->QueueDepth;

            ++PipeStatPtr;
            --PipeStatCount;
        }

        --PipeDscCount;
        ++PipeDscPtr;
    }

    CFE_SB_UnlockSharedData(__FILE__, __LINE__);

    while (PipeStatCount > 0)
    {
        memset(PipeStatPtr, 0, sizeof(*PipeStatPtr));

        ++PipeStatPtr;
        --PipeStatCount;
    }

    CFE_SB_TimeStampMsg(&CFE_SB_Global.StatTlmMsg.Hdr.Msg);
    CFE_SB_TransmitMsg(&CFE_SB_Global.StatTlmMsg.Hdr.Msg, true);

    CFE_EVS_SendEvent(CFE_SB_SND_STATS_EID,CFE_EVS_EventType_DEBUG,
                      "Software Bus Statistics packet sent");

    CFE_SB_Global.HKTlmMsg.Payload.CommandCounter++;

    return CFE_SUCCESS;
}/* CFE_SB_SendStatsCmd */


/******************************************************************************
 * \brief SB internal function to handle processing of 'Write Routing Info' Cmd
 *
 * \param[in] data Pointer to command structure
 *
 * \return Execution status, see \ref CFEReturnCodes
 */
int32 CFE_SB_WriteRoutingInfoCmd(const CFE_SB_WriteRoutingInfoCmd_t *data)
{
    char LocalFilename[OS_MAX_PATH_LEN];
    int32 Stat;

    CFE_SB_MessageStringGet(LocalFilename, data->Payload.Filename, CFE_PLATFORM_SB_DEFAULT_ROUTING_FILENAME,
            sizeof(LocalFilename), sizeof(data->Payload.Filename));

    Stat = CFE_SB_WriteRtgInfo(LocalFilename);
    CFE_SB_IncrCmdCtr(Stat);

    return CFE_SUCCESS;
}


/******************************************************************************
 * \brief SB internal function to handle processing of 'Write Pipe Info' Cmd
 *
 * \param[in] data Pointer to command structure
 *
 * \return Execution status, see \ref CFEReturnCodes
 */
int32 CFE_SB_WritePipeInfoCmd(const CFE_SB_WritePipeInfoCmd_t *data)
{
    char LocalFilename[OS_MAX_PATH_LEN];
    int32 Stat;

    CFE_SB_MessageStringGet(LocalFilename, data->Payload.Filename, CFE_PLATFORM_SB_DEFAULT_PIPE_FILENAME,
            sizeof(LocalFilename), sizeof(data->Payload.Filename));

    Stat = CFE_SB_WritePipeInfo(LocalFilename);
    CFE_SB_IncrCmdCtr(Stat);

    return CFE_SUCCESS;
}


/******************************************************************************
 * \brief SB internal function to handle processing of 'Write Map Info' Cmd
 *
 * \param[in] data Pointer to command structure
 *
 * \return Execution status, see \ref CFEReturnCodes
 */
int32 CFE_SB_WriteMapInfoCmd(const CFE_SB_WriteMapInfoCmd_t *data)
{
    char LocalFilename[OS_MAX_PATH_LEN];
    int32 Stat;

    CFE_SB_MessageStringGet(LocalFilename, data->Payload.Filename, CFE_PLATFORM_SB_DEFAULT_MAP_FILENAME,
            sizeof(LocalFilename), sizeof(data->Payload.Filename));

    Stat = CFE_SB_WriteMapInfo(LocalFilename);

    CFE_SB_IncrCmdCtr(Stat);

    return CFE_SUCCESS;
}

/******************************************************************************
 * Local callback helper for writing routing info to a file
 */
void CFE_SB_WriteRouteToFile(CFE_SBR_RouteId_t RouteId, void *ArgPtr)
{
    struct RouteInfo
    {
        CFE_SB_PipeId_t PipeId;
        uint8           Active;
        uint16          DestCnt;
    };

    CFE_SB_FileWriteCallback_t *args;
    CFE_SB_DestinationD_t      *destptr;
    CFE_SB_PipeD_t             *pipedptr;
    int32                       status;
    CFE_SB_RoutingFileEntry_t   entry;
    struct RouteInfo RouteInfo[CFE_PLATFORM_SB_MAX_DEST_PER_PKT];
    struct RouteInfo *RouteInfoPtr;
    uint32 NumDest;

    /* Cast arguments for local use */
    args = (CFE_SB_FileWriteCallback_t *)ArgPtr;

    NumDest = 0;

    /* Data must be locked to snapshot the route info */
    CFE_SB_LockSharedData(__FILE__, __LINE__);

    destptr = CFE_SBR_GetDestListHeadPtr(RouteId);
    entry.MsgId = CFE_SBR_GetMsgId(RouteId);
    RouteInfoPtr = RouteInfo;

    while((destptr != NULL) && NumDest < CFE_PLATFORM_SB_MAX_DEST_PER_PKT)
    {
        pipedptr = CFE_SB_LocatePipeDescByID(destptr->PipeId);

        /* If invalid id, continue on to next entry */
        if (CFE_SB_PipeDescIsMatch(pipedptr,destptr->PipeId))
        {
            RouteInfoPtr->PipeId = destptr->PipeId;
            RouteInfoPtr->Active = destptr->Active;
            RouteInfoPtr->DestCnt = destptr->DestCnt;
            ++RouteInfoPtr;
            ++NumDest;
        }

        destptr = destptr->Next;
    }

    CFE_SB_UnlockSharedData(__FILE__, __LINE__);

    RouteInfoPtr = RouteInfo;
    while (NumDest > 0)
    {
        entry.PipeId    = RouteInfoPtr->PipeId;
        entry.State     = RouteInfoPtr->Active;
        entry.MsgCnt    = RouteInfoPtr->DestCnt;

        entry.AppName[0] = 0;

        /*
         * NOTE: as long as CFE_ES_GetAppName() returns success, then it
         * guarantees null termination of the output.  Return code is not
         * checked here (bad) but in case of error it does not seem to touch
         * the buffer, therefore the initialization above will protect for now
         */
        CFE_ES_GetAppName(entry.AppName, pipedptr->AppId, sizeof(entry.AppName));
        CFE_SB_GetPipeName(entry.PipeName, sizeof(entry.PipeName), entry.PipeId);

        status = OS_write (args->Fd, &entry, sizeof(CFE_SB_RoutingFileEntry_t));
        if(status != sizeof(CFE_SB_RoutingFileEntry_t))
        {
            CFE_SB_FileWriteByteCntErr(args->Filename, sizeof(CFE_SB_RoutingFileEntry_t), status);
            OS_close(args->Fd);
            args->Status = CFE_SB_FILE_IO_ERR;
        }

        args->FileSize += status;
        args->EntryCount++;

        ++RouteInfoPtr;
        --NumDest;
    }
}

/******************************************************************************
**  Function:  CFE_SB_SendSubscriptionReport()
**
**  Purpose:
**    SB internal function to generate the "ONESUB_TLM" message after a subscription.
**    No-op when subscription reporting is disabled.
**
**  Arguments:
**    Payload of notification message - MsgId, PipeId, QOS
**
**  Return:
**    CFE_SUCCESS or error code
*/
int32 CFE_SB_SendSubscriptionReport(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId, CFE_SB_Qos_t Quality)
{
    CFE_SB_SingleSubscriptionTlm_t SubRptMsg;
    int32  Status;

    Status = CFE_SUCCESS;

    if (CFE_SB_Global.SubscriptionReporting == CFE_SB_ENABLE)
    {
        CFE_MSG_Init(&SubRptMsg.Hdr.Msg,
                    CFE_SB_ValueToMsgId(CFE_SB_ONESUB_TLM_MID),
                    sizeof(SubRptMsg));

        SubRptMsg.Payload.MsgId = MsgId;
        SubRptMsg.Payload.Pipe = PipeId;
        SubRptMsg.Payload.Qos = Quality;
        SubRptMsg.Payload.SubType = CFE_SB_SUBSCRIPTION;

        Status = CFE_SB_TransmitMsg(&SubRptMsg.Hdr.Msg, true);
        CFE_EVS_SendEventWithAppID(CFE_SB_SUBSCRIPTION_RPT_EID,CFE_EVS_EventType_DEBUG,CFE_SB_Global.AppId,
                "Sending Subscription Report Msg=0x%x,Pipe=%lu,Stat=0x%x",
                (unsigned int)CFE_SB_MsgIdToValue(MsgId),
                CFE_RESOURCEID_TO_ULONG(PipeId),(unsigned int)Status);
    }

    return Status;
}


/******************************************************************************
 * \brief SB internal function to write the routing information to a file
 *
 * \param[in] Filename Pointer the file name to write
 *
 * \return Execution status, see \ref CFEReturnCodes
 * \retval #CFE_SUCCESS        \copybrief CFE_SUCCESS
 * \retval #CFE_SB_FILE_IO_ERR \copybrief CFE_SB_FILE_IO_ERR
 */
int32 CFE_SB_WriteRtgInfo(const char *Filename)
{
    CFE_SB_FileWriteCallback_t  args = {0};
    int32                       Status;
    CFE_FS_Header_t             FileHdr;

    Status = OS_OpenCreate(&args.Fd, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_WRITE_ONLY);
    if(Status < OS_SUCCESS)
    {
        CFE_EVS_SendEvent(CFE_SB_SND_RTG_ERR1_EID, CFE_EVS_EventType_ERROR,
                      "Error creating file %s, stat=0x%x",
                      Filename, (unsigned int)Status);
        return CFE_SB_FILE_IO_ERR;
    }

    /* clear out the cfe file header fields, then populate description and subtype */
    CFE_FS_InitHeader(&FileHdr, "SB Routing Information", CFE_FS_SubType_SB_ROUTEDATA);

    Status = CFE_FS_WriteHeader(args.Fd, &FileHdr);
    if(Status != sizeof(CFE_FS_Header_t))
    {
        CFE_SB_FileWriteByteCntErr(Filename,sizeof(CFE_FS_Header_t),Status);
        OS_close(args.Fd);
        return CFE_SB_FILE_IO_ERR;
    }

    /* Initialize the reset of the nonzero callback argument elements */
    args.FileSize = Status;
    args.Filename = Filename;

    /* Write route info to file */
    CFE_SBR_ForEachRouteId(CFE_SB_WriteRouteToFile, &args, NULL);

    if (args.Status != 0)
    {
        return args.Status;
    }
    else
    {
        OS_close(args.Fd);
        CFE_EVS_SendEvent(CFE_SB_SND_RTG_EID,CFE_EVS_EventType_DEBUG,
                          "%s written:Size=%d,Entries=%d",
                          Filename, (int)args.FileSize, (int)args.EntryCount);
        return CFE_SUCCESS;
    }

}


/******************************************************************************
 * \brief SB internal function to write the Pipe table to a file
 *
 * \param[in] Filename Pointer the file name to write
 *
 * \return Execution status, see \ref CFEReturnCodes
 * \retval #CFE_SUCCESS        \copybrief CFE_SUCCESS
 * \retval #CFE_SB_FILE_IO_ERR \copybrief CFE_SB_FILE_IO_ERR
 */
int32 CFE_SB_WritePipeInfo(const char *Filename)
{
    uint16 i;
    osal_id_t  fd;
    int32  Status;
    uint32 FileSize = 0;
    uint32 EntryCount = 0;
    CFE_FS_Header_t FileHdr;
    CFE_SB_PipeD_t *PipeDscPtr;
    CFE_SB_PipeInfoEntry_t FileEntry;
    osal_id_t SysQueueId;

    Status = OS_OpenCreate(&fd, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_WRITE_ONLY);

    if(Status < OS_SUCCESS){
        CFE_EVS_SendEvent(CFE_SB_SND_RTG_ERR1_EID,CFE_EVS_EventType_ERROR,
                          "Error creating file %s, stat=0x%x",
                           Filename,(unsigned int)Status);
        return CFE_SB_FILE_IO_ERR;
    }/* end if */

    /* clear out the cfe file header fields, then populate description and subtype */
    CFE_FS_InitHeader(&FileHdr, "SB Pipe Information", CFE_FS_SubType_SB_PIPEDATA);
    
    Status = CFE_FS_WriteHeader(fd, &FileHdr);
    if(Status != sizeof(CFE_FS_Header_t)){
        CFE_SB_FileWriteByteCntErr(Filename,sizeof(CFE_FS_Header_t),Status);
        OS_close(fd);
        return CFE_SB_FILE_IO_ERR;
    }/* end if */

    FileSize = Status;

    /* loop through the pipe table */   
    CFE_SB_LockSharedData(__FILE__,__LINE__); 
    PipeDscPtr = CFE_SB_Global.PipeTbl;

    for (i=0;i<CFE_PLATFORM_SB_MAX_PIPES;i++)
    {
        if (CFE_SB_PipeDescIsUsed(PipeDscPtr))
        {
            /*
             * Ensure any old data in the struct has been cleared
             */
            memset(&FileEntry, 0, sizeof(FileEntry));

            /*
             * Take a "snapshot" of the PipeDsc state while locked 
             */
            FileEntry.PipeId = CFE_SB_PipeDescGetID(PipeDscPtr);            
            FileEntry.AppId = PipeDscPtr->AppId;
            FileEntry.MaxQueueDepth = PipeDscPtr->QueueDepth;
            FileEntry.CurrentQueueDepth = PipeDscPtr->CurrentDepth;
            FileEntry.PeakQueueDepth = PipeDscPtr->PeakDepth;
            FileEntry.SendErrors = PipeDscPtr->SendErrors;
            FileEntry.Opts = PipeDscPtr->Opts;
            SysQueueId = PipeDscPtr->SysQueueId;

            CFE_SB_UnlockSharedData(__FILE__,__LINE__); 

            /*
             * Gather data from other subsystems while unlocked.
             * This might fail if the pipe is deleted simultaneously while this runs, but in
             * the unlikely event that happens, the name data will simply be blank as the ID(s)
             * will not validate.
             */
            OS_GetResourceName(SysQueueId, FileEntry.PipeName, sizeof(FileEntry.PipeName));
            CFE_ES_GetAppName(FileEntry.AppName, FileEntry.AppId, sizeof(FileEntry.AppName));

            Status = OS_write (fd, &FileEntry, sizeof(FileEntry));
            if (Status != sizeof(FileEntry))
            {
                CFE_SB_FileWriteByteCntErr(Filename,sizeof(FileEntry),Status);
                OS_close(fd);
                return CFE_SB_FILE_IO_ERR;
            }/* end if */

            FileSize += Status;
            EntryCount ++;

            CFE_SB_LockSharedData(__FILE__,__LINE__); 

        }/* end if */

        ++PipeDscPtr;

    }/* end for */

    CFE_SB_UnlockSharedData(__FILE__,__LINE__); 

    OS_close(fd);

    CFE_EVS_SendEvent(CFE_SB_SND_RTG_EID,CFE_EVS_EventType_DEBUG,
                      "%s written:Size=%d,Entries=%d",
                       Filename,(int)FileSize,(int)EntryCount);

    return CFE_SUCCESS;

}

/******************************************************************************
 * Local callback helper for writing map info to a file
 */
void CFE_SB_WriteMapToFile(CFE_SBR_RouteId_t RouteId, void *ArgPtr)
{
    CFE_SB_FileWriteCallback_t *args;
    int32                       status;
    CFE_SB_MsgMapFileEntry_t    entry;

    /* Cast arguments for local use */
    args = (CFE_SB_FileWriteCallback_t *)ArgPtr;

    if(args->Status != CFE_SB_FILE_IO_ERR)
    {
        CFE_SB_LockSharedData(__FILE__,__LINE__); 
        entry.MsgId = CFE_SBR_GetMsgId(RouteId);
        entry.Index = CFE_SBR_RouteIdToValue(RouteId);
        CFE_SB_UnlockSharedData(__FILE__,__LINE__); 

        status = OS_write (args->Fd, &entry, sizeof(CFE_SB_MsgMapFileEntry_t));
        if(status != sizeof(CFE_SB_MsgMapFileEntry_t))
        {
            CFE_SB_FileWriteByteCntErr(args->Filename, sizeof(CFE_SB_MsgMapFileEntry_t), status);
            OS_close(args->Fd);
            args->Status = CFE_SB_FILE_IO_ERR;
        }

        args->FileSize += status;
        args->EntryCount++;
    }
}

/******************************************************************************
 * \brief SB internal function to write the Message Map to a file
 *
 * \param[in] Filename Pointer the file name to write
 *
 * \return Execution status, see \ref CFEReturnCodes
 * \retval #CFE_SUCCESS        \copybrief CFE_SUCCESS
 * \retval #CFE_SB_FILE_IO_ERR \copybrief CFE_SB_FILE_IO_ERR
 */
int32 CFE_SB_WriteMapInfo(const char *Filename)
{
    CFE_SB_FileWriteCallback_t args = {0};
    int32                      Status;
    CFE_FS_Header_t            FileHdr;

    Status = OS_OpenCreate(&args.Fd, Filename, OS_FILE_FLAG_CREATE | OS_FILE_FLAG_TRUNCATE, OS_WRITE_ONLY);

    if (Status < OS_SUCCESS)
    {
        CFE_EVS_SendEvent(CFE_SB_SND_RTG_ERR1_EID, CFE_EVS_EventType_ERROR,
                          "Error creating file %s, stat=0x%x",
                           Filename, (unsigned int)Status);
        return CFE_SB_FILE_IO_ERR;
    }

    /* clear out the cfe file header fields, then populate description and subtype */
    CFE_FS_InitHeader(&FileHdr, "SB Message Map Information", CFE_FS_SubType_SB_MAPDATA);

    Status = CFE_FS_WriteHeader(args.Fd, &FileHdr);
    if(Status != sizeof(CFE_FS_Header_t))
    {
        CFE_SB_FileWriteByteCntErr(Filename, sizeof(CFE_FS_Header_t), Status);
        OS_close(args.Fd);
        return CFE_SB_FILE_IO_ERR;
    }

    /* Initialize the reset of the nonzero callback argument elements */
    args.FileSize = Status;
    args.Filename = Filename;

    /* Write route info to file */
    CFE_SBR_ForEachRouteId(CFE_SB_WriteMapToFile, &args, NULL);

    if (args.Status != 0)
    {
        return args.Status;
    }
    else
    {
        OS_close(args.Fd);
        CFE_EVS_SendEvent(CFE_SB_SND_RTG_EID, CFE_EVS_EventType_DEBUG,
                          "%s written:Size=%d,Entries=%d",
                          Filename, (int)args.FileSize, (int)args.EntryCount);
        return CFE_SUCCESS;
    }
}

/******************************************************************************
 * Local callback helper for sending route subscriptions
 */
void CFE_SB_SendRouteSub(CFE_SBR_RouteId_t RouteId, void *ArgPtr)
{
    CFE_SB_DestinationD_t *destptr;
    int32                  status;

    destptr = CFE_SBR_GetDestListHeadPtr(RouteId);

    /* Loop through destinations */
    while(destptr != NULL)
    {

        if(destptr->Scope == CFE_SB_MSG_GLOBAL)
        {

            /* ...add entry into pkt */
            CFE_SB_Global.PrevSubMsg.Payload.Entry[CFE_SB_Global.PrevSubMsg.Payload.Entries].MsgId = CFE_SBR_GetMsgId(RouteId);
            CFE_SB_Global.PrevSubMsg.Payload.Entry[CFE_SB_Global.PrevSubMsg.Payload.Entries].Qos.Priority = 0;
            CFE_SB_Global.PrevSubMsg.Payload.Entry[CFE_SB_Global.PrevSubMsg.Payload.Entries].Qos.Reliability = 0;
            CFE_SB_Global.PrevSubMsg.Payload.Entries++;

            /* send pkt if full */
            if(CFE_SB_Global.PrevSubMsg.Payload.Entries >= CFE_SB_SUB_ENTRIES_PER_PKT)
            {
                CFE_SB_UnlockSharedData(__func__,__LINE__);
                status = CFE_SB_TransmitMsg(&CFE_SB_Global.PrevSubMsg.Hdr.Msg, true);
                CFE_EVS_SendEvent(CFE_SB_FULL_SUB_PKT_EID, CFE_EVS_EventType_DEBUG,
                                  "Full Sub Pkt %d Sent,Entries=%d,Stat=0x%x\n",
                                  (int)CFE_SB_Global.PrevSubMsg.Payload.PktSegment,
                                  (int)CFE_SB_Global.PrevSubMsg.Payload.Entries, (unsigned int)status);
                CFE_SB_LockSharedData(__func__,__LINE__);
                CFE_SB_Global.PrevSubMsg.Payload.Entries = 0;
                CFE_SB_Global.PrevSubMsg.Payload.PktSegment++;
            }

            /*
             * break while loop through destinations, onto next route
             * This is done because we want only one network subscription per msgid
             * Later when Qos is used, we may want to take just the highest priority
             * subscription if there are more than one
             */
            break;

        }

        /* Advance to next destination */
        destptr = destptr->Next;
    }
}

/******************************************************************************
**  Function:  CFE_SB_SendPrevSubsCmd()
**
**  Purpose:
**    SB function to build and send an SB packet containing a complete list of
**    current subscriptions.Intended to be used primarily for the Software Bus
**    Networking Application (SBN).
**
**  Arguments:
**    None
**
**  Return:
**    None
*/
int32 CFE_SB_SendPrevSubsCmd(const CFE_SB_SendPrevSubsCmd_t *data)
{
    int32 status;

    /* Take semaphore to ensure data does not change during this function */
    CFE_SB_LockSharedData(__func__,__LINE__);

    /* Initialize entry/segment tracking */
    CFE_SB_Global.PrevSubMsg.Payload.PktSegment = 1;
    CFE_SB_Global.PrevSubMsg.Payload.Entries = 0;

    /* Send subcription for each route */
    CFE_SBR_ForEachRouteId(CFE_SB_SendRouteSub, NULL, NULL);

    CFE_SB_UnlockSharedData(__func__,__LINE__);

    /* if pkt has any number of entries, send it as a partial pkt */
    if(CFE_SB_Global.PrevSubMsg.Payload.Entries > 0)
    {
        status = CFE_SB_TransmitMsg(&CFE_SB_Global.PrevSubMsg.Hdr.Msg, true);
        CFE_EVS_SendEvent(CFE_SB_PART_SUB_PKT_EID, CFE_EVS_EventType_DEBUG,
                          "Partial Sub Pkt %d Sent,Entries=%d,Stat=0x%x",
                          (int)CFE_SB_Global.PrevSubMsg.Payload.PktSegment, (int)CFE_SB_Global.PrevSubMsg.Payload.Entries,
                          (unsigned int)status);
    }

    return CFE_SUCCESS;
}/* end CFE_SB_SendPrevSubsCmd */



/******************************************************************************
**  Function:  CFE_SB_IncrCmdCtr()
**
**  Purpose:
**    SB internal function to increment the proper cmd counter based on the
**    status input. This small utility was written to eliminate duplicate code.
**
**  Arguments:
**    status - typically CFE_SUCCESS or an SB error code
**
**  Return:
**    None
*/
void CFE_SB_IncrCmdCtr(int32 status){

    if(status==CFE_SUCCESS){
      CFE_SB_Global.HKTlmMsg.Payload.CommandCounter++;
    }else{
      CFE_SB_Global.HKTlmMsg.Payload.CommandErrorCounter++;
    }/* end if */

}/* end CFE_SB_IncrCmdCtr */



/******************************************************************************
**  Function:  CFE_SB_FileWriteByteCntErr()
**
**  Purpose:
**    SB internal function to report a file write error
**
**  Arguments:
**
**
**  Return:
**    None
*/
void CFE_SB_FileWriteByteCntErr(const char *Filename,uint32 Requested,uint32 Actual){

    CFE_EVS_SendEvent(CFE_SB_FILEWRITE_ERR_EID,CFE_EVS_EventType_ERROR,
                      "File write,byte cnt err,file %s,request=%d,actual=%d",
                       Filename,(int)Requested,(int)Actual);

}/* end CFE_SB_FileWriteByteCntErr() */


/******************************************************************************
**  Function:  CFE_SB_SetSubscriptionReporting()
**
**  Purpose:
**    SB internal function to enable and disable subscription reporting.
**
**  Arguments:
**
**
**  Return:
**    None
*/
void CFE_SB_SetSubscriptionReporting(uint32 state){

    CFE_SB_Global.SubscriptionReporting = state;

}/* end CFE_SB_SetSubscriptionReporting */
