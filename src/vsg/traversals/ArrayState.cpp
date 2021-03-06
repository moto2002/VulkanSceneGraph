/* <editor-fold desc="MIT License">

Copyright(c) 2020 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/traversals/ArrayState.h>
#include <vsg/state/GraphicsPipeline.h>
#include <vsg/nodes/Geometry.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/io/Options.h>

using namespace vsg;

void ArrayState::apply(const vsg::BindGraphicsPipeline& bpg)
{
    for (auto& pipelineState : bpg.getPipeline()->getPipelineStates())
    {
        if (auto ias = pipelineState.cast<vsg::InputAssemblyState>(); ias)
        {
            topology = ias->topology;
        }
        else if (auto vas = pipelineState.cast<vsg::VertexInputState>(); vas)
        {
            for (auto& attribute : vas->getAttributes())
            {
                if (attribute.location == vertex_attribute_location)
                {
                    for (auto& binding : vas->geBindings())
                    {
                        if (attribute.binding == binding.binding)
                        {
                            vertexAttribute.binding = attribute.binding;
                            vertexAttribute.offset = attribute.offset;
                            vertexAttribute.stride = binding.stride;
                            vertexAttribute.format = attribute.format;
                        }
                    }
                }
            }
        }
    }
}

void ArrayState::apply(const vsg::Geometry& geometry)
{
    apply(geometry.firstBinding, geometry.arrays);
}

void ArrayState::apply(const vsg::VertexIndexDraw& vid)
{
    apply(vid.firstBinding, vid.arrays);
}

void ArrayState::apply(const vsg::BindVertexBuffers& bvb)
{
    apply(bvb.getFirstBinding(), bvb.getArrays());
}

void ArrayState::apply(uint32_t firstBinding, const vsg::DataList& in_arrays)
{
    if (arrays.size() < (in_arrays.size() + firstBinding)) arrays.resize(in_arrays.size() + firstBinding);
    std::copy(in_arrays.begin(), in_arrays.end(), arrays.begin() + firstBinding);

    // if the required verteAttribute is wuthin the new arrays apply the appropriate array to set up the vertices array
    if ((vertexAttribute.binding >= firstBinding) && ((vertexAttribute.binding - firstBinding) < arrays.size()))
    {
        arrays[vertexAttribute.binding]->accept(*this);
    }
}

void ArrayState::apply(const vsg::vec3Array& array)
{
    vertices = &array;
}

void ArrayState::apply(const vsg::Data& array)
{
    // array hasn't been matched to vec3Array so fallback to using a proxy array to adapt it
    if (vertexAttribute.stride > 0 && (vertexAttribute.format == VK_FORMAT_R32G32B32_SFLOAT))
    {
        if (!proxy_vertices) proxy_vertices = vsg::vec3Array::create();

        uint32_t numVertices = static_cast<uint32_t>(arrays[vertexAttribute.binding]->dataSize()) / vertexAttribute.stride;
        proxy_vertices->assign(arrays[vertexAttribute.binding], vertexAttribute.offset, vertexAttribute.stride, numVertices, array.getLayout());

        vertices = proxy_vertices;
    }
    else
    {
        vertices = nullptr;
    }
}
