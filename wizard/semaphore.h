/*
  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    http://www.wizards-toolkit.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Methods to lock and unlock semaphores.
*/
#ifndef _WIZARDSTOOLKIT_SEMAPHORE_H
#define _WIZARDSTOOLKIT_SEMAPHORE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct SemaphoreInfo
  SemaphoreInfo;

extern WizardExport SemaphoreInfo
  *AllocateSemaphoreInfo(void);

extern WizardExport void
  AcquireSemaphoreInfo(SemaphoreInfo **),
  DestroySemaphoreInfo(SemaphoreInfo **),
  RelinquishSemaphoreInfo(SemaphoreInfo *),
  SemaphoreComponentTerminus(void);

extern WizardExport WizardBooleanType
  LockSemaphoreInfo(SemaphoreInfo *),
  SemaphoreComponentGenesis(void),
  UnlockSemaphoreInfo(SemaphoreInfo *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif