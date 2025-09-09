/*****************************************************************//**
 * \file   Components.hpp
 * \brief  All components used for entities and the ECS architecture
 *		   are stored here.
 * 
 * \author Ng Jing Wen
 * \date   9 September 2025
 *********************************************************************/

/*	Position component to store x and y 
	coordinates of an entity.*/
struct Position {
	float x;
	float y;
};

/*	Velocity component to store x and y
	velocity of an entity.*/
struct Velocity {
	float xVel;
	float yVel;
};

/*	Acceleration component to store x and y 
	acceleration of an entity.*/
struct Acceleration {
	float xAccel;
	float yAccel;
};





