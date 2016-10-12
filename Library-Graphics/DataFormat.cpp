﻿/*
-----------------------------------------------------------------------------
File:        DataFormat.cpp
Copyright:   Copyright (C) 2007-2015 Poojan Prabhu. All rights reserved.
Created:     10/20/2015
Last edit:   12/15/2015
Author:      Poojan Prabhu
E-mail:      openig@compro.net

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "DataFormat.h"
#include "OIGAssert.h"
#include "CommonUtils.h"

namespace OpenIG {
	namespace Library {
		namespace Graphics {

			bool DataFormatUtils::IsFloatFormat(DataFormat format)
			{
				return format == FORMAT_R32G32B32A32_FLOAT
					|| format == FORMAT_R32G32B32_FLOAT
					|| format == FORMAT_R16G16B16A16_FLOAT
					|| format == FORMAT_R32G32_FLOAT
					|| format == FORMAT_D32_FLOAT_S8X24_UINT
					|| format == FORMAT_R32_FLOAT_X8X24_TYPELESS
					|| format == FORMAT_R11G11B10_FLOAT
					|| format == FORMAT_R16G16_FLOAT
					|| format == FORMAT_D32_FLOAT
					|| format == FORMAT_R32_FLOAT
					|| format == FORMAT_R16_FLOAT
					;
			}
			int DataFormatUtils::GetNumComponents(DataFormat format)
			{
				SWITCH_BEGIN(format)
					CASE_RETURN(FORMAT_UNKNOWN, 0);
				CASE_RETURN(FORMAT_R32G32B32A32_TYPELESS, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_UINT, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_SINT, 4);
				CASE_RETURN(FORMAT_R32G32B32_TYPELESS, 3);
				CASE_RETURN(FORMAT_R32G32B32_FLOAT, 3);
				CASE_RETURN(FORMAT_R32G32B32_UINT, 3);
				CASE_RETURN(FORMAT_R32G32B32_SINT, 3);
				CASE_RETURN(FORMAT_R16G16B16A16_TYPELESS, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_FLOAT, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_UNORM, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_UINT, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_SNORM, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_SINT, 4);
				CASE_RETURN(FORMAT_R32G32_TYPELESS, 2);
				CASE_RETURN(FORMAT_R32G32_FLOAT, 2);
				CASE_RETURN(FORMAT_R32G32_UINT, 2);
				CASE_RETURN(FORMAT_R32G32_SINT, 2);
				CASE_RETURN(FORMAT_R32G8X24_TYPELESS, 2);
				CASE_RETURN(FORMAT_D32_FLOAT_S8X24_UINT, 1);
				CASE_RETURN(FORMAT_R32_FLOAT_X8X24_TYPELESS, 1);
				CASE_RETURN(FORMAT_X32_TYPELESS_G8X24_UINT, 1);
				CASE_RETURN(FORMAT_R10G10B10A2_TYPELESS, 4);
				CASE_RETURN(FORMAT_R10G10B10A2_UNORM, 4);
				CASE_RETURN(FORMAT_R10G10B10A2_UINT, 4);
				CASE_RETURN(FORMAT_R11G11B10_FLOAT, 3);
				CASE_RETURN(FORMAT_R8G8B8A8_TYPELESS, 4);
				CASE_RETURN(FORMAT_R8G8B8A8_UNORM, 4);
				CASE_RETURN(FORMAT_R8G8B8A8_UNORM_SRGB, 4);
				CASE_RETURN(FORMAT_R8G8B8A8_UINT, 4);
				CASE_RETURN(FORMAT_R8G8B8A8_SNORM, 4);
				CASE_RETURN(FORMAT_R8G8B8A8_SINT, 4);
				CASE_RETURN(FORMAT_R16G16_TYPELESS, 2);
				CASE_RETURN(FORMAT_R16G16_FLOAT, 2);
				CASE_RETURN(FORMAT_R16G16_UNORM, 2);
				CASE_RETURN(FORMAT_R16G16_UINT, 2);
				CASE_RETURN(FORMAT_R16G16_SNORM, 2);
				CASE_RETURN(FORMAT_R16G16_SINT, 2);
				CASE_RETURN(FORMAT_R32_TYPELESS, 1);
				CASE_RETURN(FORMAT_D32_FLOAT, 1);
				CASE_RETURN(FORMAT_R32_FLOAT, 1);
				CASE_RETURN(FORMAT_R32_UINT, 1);
				CASE_RETURN(FORMAT_R32_SINT, 1);
				CASE_RETURN(FORMAT_R24G8_TYPELESS, 2);
				CASE_RETURN(FORMAT_D24_UNORM_S8_UINT, 1);
				CASE_RETURN(FORMAT_R24_UNORM_X8_TYPELESS, 1);
				CASE_RETURN(FORMAT_X24_TYPELESS_G8_UINT, 1);
				CASE_RETURN(FORMAT_R8G8_TYPELESS, 2);
				CASE_RETURN(FORMAT_R8G8_UNORM, 2);
				CASE_RETURN(FORMAT_R8G8_UINT, 2);
				CASE_RETURN(FORMAT_R8G8_SNORM, 2);
				CASE_RETURN(FORMAT_R8G8_SINT, 2);
				CASE_RETURN(FORMAT_R16_TYPELESS, 1);
				CASE_RETURN(FORMAT_R16_FLOAT, 1);
				CASE_RETURN(FORMAT_D16_UNORM, 1);
				CASE_RETURN(FORMAT_R16_UNORM, 1);
				CASE_RETURN(FORMAT_R16_UINT, 1);
				CASE_RETURN(FORMAT_R16_SNORM, 1);
				CASE_RETURN(FORMAT_R16_SINT, 1);
				CASE_RETURN(FORMAT_R8_TYPELESS, 1);
				CASE_RETURN(FORMAT_R8_UNORM, 1);
				CASE_RETURN(FORMAT_R8_UINT, 1);
				CASE_RETURN(FORMAT_R8_SNORM, 1);
				CASE_RETURN(FORMAT_R8_SINT, 1);
				CASE_RETURN(FORMAT_A8_UNORM, 1);
				CASE_RETURN(FORMAT_R1_UNORM, 1);
				CASE_RETURN(FORMAT_R9G9B9E5_SHAREDEXP, 4);
				CASE_RETURN(FORMAT_R8G8_B8G8_UNORM, 4);
				CASE_RETURN(FORMAT_G8R8_G8B8_UNORM, 4);
				CASE_RETURN(FORMAT_BC1_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC1_UNORM, 1);
				CASE_RETURN(FORMAT_BC1_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_BC2_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC2_UNORM, 1);
				CASE_RETURN(FORMAT_BC2_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_BC3_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC3_UNORM, 1);
				CASE_RETURN(FORMAT_BC3_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_BC4_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC4_UNORM, 1);
				CASE_RETURN(FORMAT_BC4_SNORM, 1);
				CASE_RETURN(FORMAT_BC5_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC5_UNORM, 1);
				CASE_RETURN(FORMAT_BC5_SNORM, 1);
				CASE_RETURN(FORMAT_B5G6R5_UNORM, 3);
				CASE_RETURN(FORMAT_B5G5R5A1_UNORM, 4);
				CASE_RETURN(FORMAT_B8G8R8A8_UNORM, 4);
				CASE_RETURN(FORMAT_B8G8R8X8_UNORM, 4);
				CASE_RETURN(FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 3);
				CASE_RETURN(FORMAT_B8G8R8A8_TYPELESS, 4);
				CASE_RETURN(FORMAT_B8G8R8A8_UNORM_SRGB, 4);
				CASE_RETURN(FORMAT_B8G8R8X8_TYPELESS, 4);
				CASE_RETURN(FORMAT_B8G8R8X8_UNORM_SRGB, 4);
				CASE_RETURN(FORMAT_BC6H_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC6H_UF16, 1);
				CASE_RETURN(FORMAT_BC6H_SF16, 1);
				CASE_RETURN(FORMAT_BC7_TYPELESS, 1);
				CASE_RETURN(FORMAT_BC7_UNORM, 1);
				CASE_RETURN(FORMAT_BC7_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_FORCE_UINT, 0);
				SWITCH_END(format)

					ASSERT_PREDICATE(false);
				return 0;
			}

			int DataFormatUtils::GetEachComponentSizeInBytes(DataFormat format)
			{
				SWITCH_BEGIN(format)
					CASE_RETURN(FORMAT_UNKNOWN, 0);
				CASE_RETURN(FORMAT_R32G32B32A32_TYPELESS, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_UINT, 4);
				CASE_RETURN(FORMAT_R32G32B32A32_SINT, 4);
				CASE_RETURN(FORMAT_R32G32B32_TYPELESS, 4);
				CASE_RETURN(FORMAT_R32G32B32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32G32B32_UINT, 4);
				CASE_RETURN(FORMAT_R32G32B32_SINT, 4);
				CASE_RETURN(FORMAT_R16G16B16A16_TYPELESS, 2);
				CASE_RETURN(FORMAT_R16G16B16A16_FLOAT, 2);
				CASE_RETURN(FORMAT_R16G16B16A16_UNORM, 2);
				CASE_RETURN(FORMAT_R16G16B16A16_UINT, 2);
				CASE_RETURN(FORMAT_R16G16B16A16_SNORM, 2);
				CASE_RETURN(FORMAT_R16G16B16A16_SINT, 2);
				CASE_RETURN(FORMAT_R32G32_TYPELESS, 4);
				CASE_RETURN(FORMAT_R32G32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32G32_UINT, 4);
				CASE_RETURN(FORMAT_R32G32_SINT, 4);
				CASE_RETURN(FORMAT_R32G8X24_TYPELESS, 4);
				CASE_RETURN(FORMAT_D32_FLOAT_S8X24_UINT, 4);
				CASE_RETURN(FORMAT_R32_FLOAT_X8X24_TYPELESS, 4);
				CASE_RETURN(FORMAT_X32_TYPELESS_G8X24_UINT, 4);
				CASE_RETURN(FORMAT_R10G10B10A2_TYPELESS, 0);
				CASE_RETURN(FORMAT_R10G10B10A2_UNORM, 0);
				CASE_RETURN(FORMAT_R10G10B10A2_UINT, 0);
				CASE_RETURN(FORMAT_R11G11B10_FLOAT, 0);
				CASE_RETURN(FORMAT_R8G8B8A8_TYPELESS, 1);
				CASE_RETURN(FORMAT_R8G8B8A8_UNORM, 1);
				CASE_RETURN(FORMAT_R8G8B8A8_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_R8G8B8A8_UINT, 1);
				CASE_RETURN(FORMAT_R8G8B8A8_SNORM, 1);
				CASE_RETURN(FORMAT_R8G8B8A8_SINT, 1);
				CASE_RETURN(FORMAT_R16G16_TYPELESS, 2);
				CASE_RETURN(FORMAT_R16G16_FLOAT, 2);
				CASE_RETURN(FORMAT_R16G16_UNORM, 2);
				CASE_RETURN(FORMAT_R16G16_UINT, 2);
				CASE_RETURN(FORMAT_R16G16_SNORM, 2);
				CASE_RETURN(FORMAT_R16G16_SINT, 2);
				CASE_RETURN(FORMAT_R32_TYPELESS, 4);
				CASE_RETURN(FORMAT_D32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32_FLOAT, 4);
				CASE_RETURN(FORMAT_R32_UINT, 4);
				CASE_RETURN(FORMAT_R32_SINT, 4);
				CASE_RETURN(FORMAT_R24G8_TYPELESS, 3);
				CASE_RETURN(FORMAT_D24_UNORM_S8_UINT, 3);
				CASE_RETURN(FORMAT_R24_UNORM_X8_TYPELESS, 3);
				CASE_RETURN(FORMAT_X24_TYPELESS_G8_UINT, 3);
				CASE_RETURN(FORMAT_R8G8_TYPELESS, 1);
				CASE_RETURN(FORMAT_R8G8_UNORM, 1);
				CASE_RETURN(FORMAT_R8G8_UINT, 1);
				CASE_RETURN(FORMAT_R8G8_SNORM, 1);
				CASE_RETURN(FORMAT_R8G8_SINT, 1);
				CASE_RETURN(FORMAT_R16_TYPELESS, 2);
				CASE_RETURN(FORMAT_R16_FLOAT, 2);
				CASE_RETURN(FORMAT_D16_UNORM, 2);
				CASE_RETURN(FORMAT_R16_UNORM, 2);
				CASE_RETURN(FORMAT_R16_UINT, 2);
				CASE_RETURN(FORMAT_R16_SNORM, 2);
				CASE_RETURN(FORMAT_R16_SINT, 2);
				CASE_RETURN(FORMAT_R8_TYPELESS, 1);
				CASE_RETURN(FORMAT_R8_UNORM, 1);
				CASE_RETURN(FORMAT_R8_UINT, 1);
				CASE_RETURN(FORMAT_R8_SNORM, 1);
				CASE_RETURN(FORMAT_R8_SINT, 1);
				CASE_RETURN(FORMAT_A8_UNORM, 1);
				CASE_RETURN(FORMAT_R1_UNORM, 0);
				CASE_RETURN(FORMAT_R9G9B9E5_SHAREDEXP, 0);
				CASE_RETURN(FORMAT_R8G8_B8G8_UNORM, 1);
				CASE_RETURN(FORMAT_G8R8_G8B8_UNORM, 1);
				CASE_RETURN(FORMAT_BC1_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC1_UNORM, 0);
				CASE_RETURN(FORMAT_BC1_UNORM_SRGB, 0);
				CASE_RETURN(FORMAT_BC2_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC2_UNORM, 0);
				CASE_RETURN(FORMAT_BC2_UNORM_SRGB, 0);
				CASE_RETURN(FORMAT_BC3_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC3_UNORM, 0);
				CASE_RETURN(FORMAT_BC3_UNORM_SRGB, 0);
				CASE_RETURN(FORMAT_BC4_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC4_UNORM, 0);
				CASE_RETURN(FORMAT_BC4_SNORM, 0);
				CASE_RETURN(FORMAT_BC5_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC5_UNORM, 0);
				CASE_RETURN(FORMAT_BC5_SNORM, 0);
				CASE_RETURN(FORMAT_B5G6R5_UNORM, 0);
				CASE_RETURN(FORMAT_B5G5R5A1_UNORM, 0);
				CASE_RETURN(FORMAT_B8G8R8A8_UNORM, 1);
				CASE_RETURN(FORMAT_B8G8R8X8_UNORM, 1);
				CASE_RETURN(FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0);
				CASE_RETURN(FORMAT_B8G8R8A8_TYPELESS, 1);
				CASE_RETURN(FORMAT_B8G8R8A8_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_B8G8R8X8_TYPELESS, 1);
				CASE_RETURN(FORMAT_B8G8R8X8_UNORM_SRGB, 1);
				CASE_RETURN(FORMAT_BC6H_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC6H_UF16, 0);
				CASE_RETURN(FORMAT_BC6H_SF16, 0);
				CASE_RETURN(FORMAT_BC7_TYPELESS, 0);
				CASE_RETURN(FORMAT_BC7_UNORM, 0);
				CASE_RETURN(FORMAT_BC7_UNORM_SRGB, 0);
				CASE_RETURN(FORMAT_FORCE_UINT, 0);
				SWITCH_END(format)

					ASSERT_PREDICATE(false);
				return 0;
			}

		}
	}
}
