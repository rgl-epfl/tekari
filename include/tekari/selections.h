#pragma once

#include <nanogui/opengl.h>
#include <tbb/parallel_for.h>
#include "points_stats.h"
#include "Metadata.h"

TEKARI_NAMESPACE_BEGIN

enum SelectionMode
{
	STANDARD,
	ADD,
	SUBTRACT
};

struct SelectionBox
{
	Vector2i topLeft;
	Vector2i size;

	inline bool empty() const { return size[0] == 0 || size[1] == 0; }

	inline bool contains(const Vector2i& point) const
	{
		return  point[0] >= topLeft[0] && point[0] <= topLeft[0] + size[0] &&
			point[1] >= topLeft[1] && point[1] <= topLeft[1] + size[1];
	}
};

extern void select_points(
	const MatrixXf &V2D,
	const VectorXf &H,
	VectorXu8 &selectedPoints,
	const Matrix4f & mvp,
	const SelectionBox& selectionBox,
	const Vector2i & canvasSize,
	SelectionMode mode
);

extern void select_closest_point(
	const MatrixXf &V2D,
	const VectorXf &H,
	VectorXu8 &selectedPoints,
	const Matrix4f& mvp,
	const Vector2i & mousePos,
	const Vector2i & canvasSize
);

extern void select_highest_point(
	const PointsStats &pointsInfo,
	const PointsStats &selectionInfo,
	VectorXu8 &selectedPoints,
	unsigned int waveLengthIndex
);

extern void select_all_points(VectorXu8 &selectedPoints);
extern void deselect_all_points(VectorXu8 &selectedPoints);

extern void move_selection_along_path(bool up, VectorXu8 &selectedPoints);

extern void delete_selected_points(
	VectorXu8 &selectedPoints,
	MatrixXf &rawPoints,
	MatrixXf &V2D,
	PointsStats &selectionInfo,
	Metadata& metadata
);

extern unsigned int count_selected_points(
	const VectorXu8 &selectedPoints
);

TEKARI_NAMESPACE_END