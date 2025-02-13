// Copyright Epic Games, Inc. All Rights Reserved.

import { action, makeObservable, observable } from 'mobx';
import { observer } from 'mobx-react-lite';
import { Callout, DirectionalHint, List, Stack, Text } from "@fluentui/react";
import React from "react";
import { Link } from "react-router-dom";
import { GetJobResponse, GetLabelStateResponse, StepData } from "../backend/Api";
import { JobDetails } from "../backend/JobDetails";
import { StepStatusIcon } from './StatusIcon';
import backend from 'horde/backend';

type StepItem = {
   step: StepData;
};

export type CalloutState = {
   jobId?: string;
   target?: string;
   label?: GetLabelStateResponse;
}

export class CalloutController {

   constructor() {
      makeObservable(this);
   }

   setState(state: CalloutState | undefined, now: boolean = false) {

      if (!state) {

         clearTimeout(this.setTimeout);
         clearTimeout(this.closeTimeout);
         this.setTimeout = this.closeTimeout = undefined;
         this.pending = undefined;

         if (now) {
            this.setStateInternal({});
            this.job = undefined;
         } else {
            this.closeTimeout = setTimeout(() => {
               this.setStateInternal({});
               this.job = undefined;
            }, 600);

         }

         return;

      }

      // check pending state
      if (state.jobId === this.pending?.jobId && state.target === this.pending?.target) {
         return;
      }

      // check current state
      if (state.jobId === this.state.jobId && state.target === this.state.target) {
         return;
      }


      clearTimeout(this.setTimeout);
      clearTimeout(this.closeTimeout);
      this.closeTimeout = undefined;
      this.pending = state;

      this.setTimeout = setTimeout(() => {

         this.setTimeout = undefined;
         this.pending = undefined;

         if (this.job?.id === state.jobId) {
            this.setStateInternal(state);
            return;
         }

         if (state.jobId) {
            let job = this.jobCache.get(state.jobId);
            const time = this.jobCacheTime.get(state.jobId);

            if (job && time) {
               if ((Date.now() - time.getTime()) / 1000 > 120) {
                  job = undefined;                  
               }
            }

            if (job) {
               this.job = job;
               this.setStateInternal(state);
            } else {
               backend.getJob(state.jobId!).then(job => {
                  this.job = job;
                  this.jobCache.set(job.id, job);
                  this.jobCacheTime.set(job.id, new Date());
                  this.setStateInternal(state);
               })      
            }
         }

      }, 800);

   }

   @action
   private setStateInternal(state: CalloutState) {
      this._state = { ...state };
      this.stateUpdated++;
   }

   @action
   clear() {
      this.job = undefined;
      this._state = {};
      this.stateUpdated++;
   }

   @observable
   stateUpdated = 0;

   get state(): CalloutState {
      // subscribe
      if (this.stateUpdated) { }
      return this._state;
   }

   private _state: CalloutState = {};

   pending?: CalloutState;
   job?: GetJobResponse;
   jobCache: Map<string, GetJobResponse> = new Map();
   jobCacheTime: Map<string, Date> = new Map();

   setTimeout: any;
   closeTimeout: any;
}

export const JobLabelCallout: React.FC<{ controller: CalloutController }> = observer(({ controller }) => {

   const state = controller.state;

   const label = state.label;
   const jobId = state.jobId;
   const target = state.target;
   const job = controller.job;

   if (!label || !job) {
      return <div />;
   }

   const allSteps = (job.batches ?? []).map(b => b.steps ?? []).flat();
   const steps = allSteps.filter(step => label.steps.indexOf(step.id) !== -1);
   const items = steps.map(step => { return { step: step }; });

   const onRenderCell = (stepItem?: StepItem): JSX.Element => {

      const step = stepItem?.step;

      if (!step) {
         return <div />;
      }

      const stepUrl = `/job/${jobId}?step=${step.id}`;

      const stepName = step.name;

      return <Stack tokens={{ childrenGap: 12 }} >
         <Stack horizontal>
            <Link to={stepUrl} onClick={(ev) => { ev.stopPropagation(); }}><div style={{ cursor: "pointer" }}>
               <Stack horizontal>
                  <StepStatusIcon step={step} style={{ fontSize: 10 }} />
                  <Text styles={{ root: { fontSize: 10, paddingRight: 4, paddingTop: 0, userSelect: "none" } }}>{`${stepName}`}</Text>
               </Stack>
            </div></Link>
         </Stack>
      </Stack>;
   };


   return <Callout isBeakVisible={true}
      onDismiss={() => { controller.setState(undefined) }}
      beakWidth={12}
      hidden={false}
      target={target}
      role="alertdialog"
      gapSpace={8}
      setInitialFocus={true}
      shouldRestoreFocus={false}
      directionalHint={DirectionalHint.bottomCenter}>
      <Stack styles={{ root: { paddingTop: '4x', paddingLeft: '14px', paddingBottom: '20px', paddingRight: '20px' } }}
         onMouseMove={(ev) => ev.stopPropagation()}
         onMouseOver={(ev) => {
            clearTimeout(controller.closeTimeout)
         }}
         onMouseLeave={(ev) => {
            controller.setState(undefined, true);
         }}>
         <Stack styles={{ root: { paddingTop: 12, paddingLeft: 4 } }}>
            <List id="steplist" items={items}
               onRenderCell={onRenderCell}
               data-is-focusable={false} />
         </Stack>
      </Stack>
   </Callout>
});