#include "scripting/script_renderer.h"
#include "world.h"
#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
#include "scripting/script_node_type.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/transform_2d_component.h"

ScriptRenderer::ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: resources(resources)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
	, nativeZoom(nativeZoom)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
}

void ScriptRenderer::setGraph(const ScriptGraph* graph)
{
	this->graph = graph;
}

void ScriptRenderer::setState(const ScriptState* scriptState)
{
	this->state = scriptState;
}

void ScriptRenderer::draw(Painter& painter, Vector2f basePos, float curZoom)
{
	if (!graph) {
		return;
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	for (const auto& node: graph->getNodes()) {
		drawNodeOutputs(painter, basePos, node, *graph, effectiveZoom);
	}

	if (currentPath) {
		drawConnection(painter, currentPath.value(), curZoom);
	}
	
	for (uint32_t i = 0; i < static_cast<uint32_t>(graph->getNodes().size()); ++i) {
		const auto& node = graph->getNodes()[i];

		const bool highlightThis = highlightNode && highlightNode->nodeId == i;
		
		NodeDrawMode mode = NodeDrawMode::Normal;
		if (highlightThis && highlightNode->elementType == NodeElementType::Node) {
			mode = NodeDrawMode::Highlight;
		} else if (state && !state->hasThreadAt(i)) {
			mode = NodeDrawMode::Dimmed;
		}
		
		drawNode(painter, basePos, node, effectiveZoom, mode, highlightThis ? highlightNode->elementType : std::optional<NodeElementType>(), highlightThis ? highlightNode->elementId : 0);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom)
{
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	for (size_t i = 0; i < node.getOutputs().size(); ++i) {
		const auto& output = node.getOutputs()[i];
		if (!output.nodeId) {
			continue;
		}
		
		const size_t srcIdx = i;
		const Vector2f srcPos = getNodeElementArea(*nodeType, NodeElementType::Output, basePos, node, srcIdx, curZoom).getCentre();

		const size_t dstIdx = output.inputPin;
		const auto& dstNode = graph.getNodes().at(output.nodeId.value());
		const auto* dstNodeType = nodeTypeCollection.tryGetNodeType(dstNode.getType());
		if (!dstNodeType) {
			continue;
		}
		const Vector2f dstPos = getNodeElementArea(*dstNodeType, NodeElementType::Input, basePos, dstNode, dstIdx, curZoom).getCentre();
		
		drawConnection(painter, ConnectionPath{ srcPos, dstPos, NodeElementType::Output }, curZoom);
	}

	for (size_t i = 0; i < node.getTargets().size(); ++i) {
		const auto& target = node.getTargets()[i];
		if (target.isValid()) {
			auto entity = world.getEntity(target);
			auto* transform = entity.tryGetComponent<Transform2DComponent>();
			if (transform) {
				const Vector2f srcPos = getNodeElementArea(*nodeType, NodeElementType::Target, basePos, node, i, curZoom).getCentre();
				const auto dstPos = transform->getGlobalPosition();

				drawConnection(painter, ConnectionPath{ srcPos, dstPos, NodeElementType::Target }, curZoom);
			}
		} else {
			Logger::logWarning("Invalid target on script graph node");
		}
	}
}

BezierCubic ScriptRenderer::makeBezier(const ConnectionPath& path) const
{
	const float xSelect = path.type == NodeElementType::Target ? 0.0f : (path.type == NodeElementType::Output ? 1.0f : -1.0f);
	const float ySelect = 1.0f - std::abs(xSelect);
	const Vector2f axisSelector = Vector2f(xSelect, ySelect);
	
	const float dist = std::max(std::abs((path.to * axisSelector).manhattanLength() - (path.from * axisSelector).manhattanLength()), 20.0f) / 2;
	
	return BezierCubic(path.from, path.from + dist * axisSelector, path.to - dist * axisSelector, path.to);
}

void ScriptRenderer::drawConnection(Painter& painter, const ConnectionPath& path, float curZoom) const
{
	const auto col = path.type == NodeElementType::Target ? Colour4f(0.35f, 1.0f, 0.35f) : Colour4f(1, 1, 1);
	painter.drawLine(makeBezier(path), 1.5f / curZoom, col);
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<NodeElementType> highlightElement, uint8_t highlightElementId)
{
	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto pos = basePos + node.getPosition();

	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	const auto baseCol = getNodeColour(*nodeType);
	Colour4f col = baseCol;
	switch (drawMode) {
	case NodeDrawMode::Highlight:
		col = col.inverseMultiplyLuma(0.5f);
		break;
	case NodeDrawMode::Dimmed:
		col = col.multiplyLuma(0.5f);
		break;
	}
	
	// Node body
	nodeBg.clone()
		.setColour(col)
		.setPosition(pos)
		.scaleTo(nodeSize + border)
		.setSize(nodeBg.getSize() / curZoom)
		.setSliceScale(1.0f / curZoom)
		//.setTexRect(nodeBg.getTexRect() / curZoom)
		.draw(painter);

	getIcon(*nodeType).clone()
		.setPosition(pos)
		.setScale(1.0f / curZoom)
		.draw(painter);

	// Draw pins
	const uint8_t nPins[] = { nodeType->getNumInputPins(), nodeType->getNumOutputPins(), nodeType->getNumTargetPins() };
	const NodeElementType types[] = { NodeElementType::Input, NodeElementType::Output, NodeElementType::Target };
	const Colour4f colours[] = { Colour4f(0.8f, 0.8f, 0.8f), Colour4f(0.8f, 0.8f, 0.8f), Colour4f(0.35f, 1, 0.35f) };
	for (size_t i = 0; i < 3; ++i) {
		for (size_t j = 0; j < nPins[i]; ++j) {
			const auto circle = getNodeElementArea(*nodeType, types[i], basePos, node, j, curZoom);
			const auto baseCol = colours[i];
			const auto col = highlightElement == types[i] && highlightElementId == j ? baseCol.inverseMultiplyLuma(0.3f) : baseCol;
			pinSprite.clone()
				.setPosition(circle.getCentre())
				.setColour(col)
				.setScale(1.0f / curZoom)
				.draw(painter);
		}
	}
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}

Circle ScriptRenderer::getNodeElementArea(const IScriptNodeType& nodeType, NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom) const
{
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto getOffset = [&] (size_t idx, size_t n)
	{
		const float spacing = 10.0f / curZoom;
		return (static_cast<float>(idx) + (n - 1) * 0.5f) * spacing;
	};
	
	Vector2f offset;
	switch (type) {
	case NodeElementType::Input:
		offset = Vector2f(-nodeSize.x * 0.5f, getOffset(elemIdx, nodeType.getNumInputPins()));
		break;
	case NodeElementType::Output:
		offset = Vector2f(nodeSize.x * 0.5f, getOffset(elemIdx, nodeType.getNumOutputPins()));
		break;
	case NodeElementType::Target:
		offset = Vector2f(getOffset(elemIdx, nodeType.getNumTargetPins()), nodeSize.y * 0.5f);
		break;
	}

	const Vector2f pos = basePos + node.getPosition();
	const Vector2f centre = pos + offset / curZoom;
	const float radius = 4.0f / curZoom;
	
	return Circle(centre, radius);
}

Colour4f ScriptRenderer::getNodeColour(const IScriptNodeType& nodeType) const
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Condition:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.35f, 0.97f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

const Sprite& ScriptRenderer::getIcon(const IScriptNodeType& nodeType)
{
	const auto iter = icons.find(nodeType.getId());
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[nodeType.getId()] = Sprite().setImage(resources, nodeType.getIconName()).setPivot(Vector2f(0.5f, 0.5f));
	return icons[nodeType.getId()];
}

std::optional<ScriptRenderer::NodeUnderMouseInfo> ScriptRenderer::getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos) const
{
	if (!graph || !mousePos) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	const auto nodeSize = getNodeSize(effectiveZoom);
	const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;

	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();

		const auto nodeBounds = Circle(pos, area.getSize().length() / 2);
		if (!nodeBounds.contains(mousePos.value())) {
			continue;
		}
		
		const auto nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto curRect = area + pos;
		
		// Check each pin handle
		const uint8_t nPins[] = { nodeType->getNumInputPins(), nodeType->getNumOutputPins(), nodeType->getNumTargetPins() };
		const NodeElementType types[] = { NodeElementType::Input, NodeElementType::Output, NodeElementType::Target };
		for (size_t j = 0; j < 3; ++j) {
			for (uint8_t k = 0; k < nPins[j]; ++k) {
				const auto circle = getNodeElementArea(*nodeType, types[j], basePos, node, k, curZoom).expand(4.0f / curZoom);
				if (circle.contains(mousePos.value())) {
					return NodeUnderMouseInfo{ static_cast<uint32_t>(i), types[j], k, curRect, circle.getCentre() };
				}
			}
		}
		
		// Check main body
		if (curRect.contains(mousePos.value())) {
			return NodeUnderMouseInfo{ static_cast<uint32_t>(i), NodeElementType::Node, 0, curRect, Vector2f() };
		}
	}

	return {};
}

void ScriptRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node)
{
	highlightNode = node;
}

void ScriptRenderer::setCurrentPath(std::optional<ConnectionPath> path)
{
	currentPath = path;
}