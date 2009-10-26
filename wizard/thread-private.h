/*
  Copyright 1999-2009 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    http://www.imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  WizardCore private methods for internal threading.
*/
#ifndef _WIZARDSTOOLKIT_THREAD_PRIVATE_H
#define _WIZARDSTOOLKIT_THREAD_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <wizard/thread_.h>

#if defined(WIZARDSTOOLKIT_HAVE_PTHREAD)
  typedef pthread_mutex_t WizardMutexType;
#elif defined(__WINDOWS__)
  typedef CRITICAL_SECTION WizardMutexType;
#else
  typedef unsigned long WizardMutexType;
#endif

static inline WizardThreadType GetWizardThreadId(void)
{
#if defined(WIZARDSTOOLKIT_HAVE_PTHREAD)
  return(pthread_self());
#elif defined(__WINDOWS__)
  return(GetCurrentThreadId());
#else
  return(getpid());
#endif
}

static inline unsigned long GetWizardThreadSignature(void)
{
#if defined(MAGICKCORE_HAVE_PTHREAD)
  {
    union
    {
      pthread_t
        id;

      unsigned long
        signature;
    } wizard_thread;

    wizard_thread.signature=0UL;
    wizard_thread.id=pthread_self();
    return(wizard_thread.signature);
  }
#elif defined(__WINDOWS__)
  return((unsigned long) GetCurrentThreadId());
#else
  return((unsigned long) getpid());
#endif
}

static inline WizardBooleanType IsWizardThreadEqual(const WizardThreadType id)
{
#if defined(WIZARDSTOOLKIT_HAVE_PTHREAD)
  if (pthread_equal(id,pthread_self()) != 0)
    return(WizardTrue);
#elif defined(__WINDOWS__)
  if (id == GetCurrentThreadId())
    return(WizardTrue);
#else
  if (id == getpid())
    return(WizardTrue);
#endif
  return(WizardFalse);
}

/*
  Lightweight OpenMP methods.
*/
static inline unsigned long GetOpenMPMaximumThreads(void)
{
  static unsigned long
    maximum_threads = 1;

#if defined(WIZARDSTOOLKIT_OPENMP_SUPPORT)
  if (omp_get_max_threads() > (long) maximum_threads)
    maximum_threads=omp_get_max_threads();
#endif
  return(maximum_threads);
}

static inline long GetOpenMPThreadId(void)
{
#if defined(WIZARDSTOOLKIT_OPENMP_SUPPORT)
  return(omp_get_thread_num());
#else
  return(0);
#endif
}

static inline void SetOpenMPMaximumThreads(const unsigned long threads)
{
#if defined(WIZARDSTOOLKIT_OPENMP_SUPPORT)
  omp_set_num_threads(threads);
#else
  (void) threads;
#endif
}

static inline void SetOpenMPNested(const int value)
{
#if defined(MAGICKCORE_OPENMP_SUPPORT) && (_OPENMP >= 200203)
  omp_set_nested(value);
#else
  (void) value;
#endif
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif