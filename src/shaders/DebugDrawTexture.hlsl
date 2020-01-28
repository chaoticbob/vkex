/*
 Copyright 2018 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

struct VSInput {
  float4  PositionOS : PositionOS;
  float2  TexCoord   : TexCoord;
};

struct VSOutput {
  float4  PositionCS : SV_Position;
  float2  TexCoord   : TexCoord;
};


VSOutput vsmain(VSInput input)
{
  VSOutput output = (VSOutput)0;
  output.PositionCS = input.PositionOS;
  output.TexCoord = input.TexCoord;
  return output;
}

struct PSInput {
  float4  PositionCS : SV_Position;
  float2  TexCoord   : TexCoord;
};

Texture2D    Tex0     : register(t0);
SamplerState Sampler0 : register(s1);

float4 psmain(PSInput input) : SV_Target
{
  float4 color = Tex0.Sample(Sampler0, input.TexCoord);
  return color;
}