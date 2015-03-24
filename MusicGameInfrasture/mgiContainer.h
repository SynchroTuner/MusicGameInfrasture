/*
Copyright(c) 2015, SynchroTuner
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

*Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and / or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**
\file mgiContainer.h

Note container header file.
*/
#ifndef _MGI_CONTAINER_H
#define _MGI_CONTAINER_H

#include "mgiConfig.h"
#include <new>
#include <stddef.h>
#include <type_traits>

#ifndef _MSC_VER
#define __alignof(x) __alignof__(x)
#endif

/** Root namespace for Music Game Infrasture.*/
namespace mgi{
	/** Container class, which may be allocated sequentially many times, and
	deallocate everything at one time. This class only allocate space and call
	the constructor. It does NOT call the destructor of its
	elements inside. It's the user's responsibility to call them.

	\tparam nBlockSize The internal list block size.Must > 0.*/
	template <unsigned int nBlockSize = 4096 - sizeof(void*)> class CSeqAllocCont
	{
		static_assert(nBlockSize > 128, "nBlockSize must be > 128");
	private:
		struct NODE_t{
			char data[nBlockSize];
			NODE_t* next;
		} *pHead, *pLast;
		uintptr_t pCurrent;
		CSeqAllocCont(const CSeqAllocCont&); //uncopyable
	public:
		/** Default constructor.*/
		CSeqAllocCont() :pHead(new NODE_t{ { 0 }, nullptr }), pLast(pHead), pCurrent((uintptr_t)pHead){}
		/** 
		Allocate space and call the constructor to construct the object.
		However, it's the user's responsibility to determine whether the objects
		should be freed or not, and to call destructor on the pointers.

		\tparam T The type of the object to allocate.
		\param args Arguments for the constructor.
		\return The allocated address.
		*/
		template<class T, class... Args> T* Allocate(Args&&... args)
		{
			static_assert(sizeof(T) <= nBlockSize, "nBlockSize is less than the size of T. Make it bigger.");
			if ((pCurrent & __alignof(T)) != 0) //confirm alignment requirement
			{
				pCurrent &= __alignof(T);
				pCurrent += __alignof(T);
			}
			if (pCurrent + sizeof(T) > (uintptr_t)&pLast->data[nBlockSize]) //exceeds current node. allocate a new block.
			{
				if(pLast->next==nullptr) pLast->next = new NODE_t{ { 0 }, nullptr };
				pLast = pLast->next;
				pCurrent = (uintptr_t)pLast;
			}
			if ((pCurrent & __alignof(T)) != 0) //confirm alignment requirement again
			{
				pCurrent &= __alignof(T);
				pCurrent += __alignof(T);
				
				if (pCurrent + sizeof(T) > (uintptr_t)&pLast->data[nBlockSize])
					throw std::bad_alloc();//this allocation would never succeed.try larger nBlockSize.
			}
			void* pRet = (void*)pCurrent;
			pCurrent += sizeof(T);
			return new(pRet)T(std::forward<Args>(args)...);
		}

		/**
		Make this container empty without free the internal space it allocated, which is for reuse.
		*/
		void Clean()
		{
			pLast = pHead;
			pCurrent = (uintptr_t)pHead;
		}

		/**
		Empty the container, and also free the redundant internal space allocated.
		*/
		void Reset()
		{
			pLast = pHead->next;
			pHead->next = nullptr;
			while(pLast != nullptr){
				NODE_t* pTemp = pLast;
				pLast = pLast->next;
				delete pTemp;
			}
		}
		~CSeqAllocCont()
		{
			Reset();
			delete pHead;
		}
	};
}
#endif