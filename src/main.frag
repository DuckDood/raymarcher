#version 330 core

out vec4 FragColor;

uniform float uTime;
uniform vec2 uResolution;

uniform mat4 uCamRotationMatrix;
uniform vec3 uCamPosition;

/*struct sphere {
	vec3 position;
	float radius;
};

struct rect {
	vec3 position;
	vec3 size;
};*/

struct camRay {
	vec3 direction;
	vec3 position;
};

struct shape {
	mat4 transformation;
	vec4 params;
	uint type;
};

const uint SHAPE_TYPE_SPHERE = uint(0);
const uint SHAPE_TYPE_RECT = uint(1);


float getSphereDistance(vec3 position, shape object) {
	return length((inverse(object.transformation) * vec4(position, 1.0)).xyz) - object.params.x;
	//return length(vec4(position, 1.0).xyz) - object.params.x;
}

float getRectDistance(vec3 position, shape object) {
	// general implementation from sebastian lague's video on ray marching
	vec3 off = abs((inverse(object.transformation) * vec4(position, 1.0)).xyz) - object.params.xyz;

	float unsignedDist = length(max(off, 0.0));

	float dstInsideBox = max(max(min(off.x,0.0),min(off.y,0.0)),min(off.z,0.0));

	return unsignedDist + dstInsideBox;
}

float getShapeDistance(vec3 position, shape object) {
	switch(object.type) {
		case SHAPE_TYPE_SPHERE:
			return getSphereDistance(position, object);
			break;
		case SHAPE_TYPE_RECT:
			return getRectDistance(position, object);
			break;
	}
	return 0.0; // sigh
}

layout (std140) uniform sph
{
	int shapeCount;
	shape shapes[5];
};


/*shape shapes[2] = shape[](
	shape(
			vec3(0.0, cos(uTime), 0.0),
			vec4(1.0, 0.0, 0.0, 0.0),
			SHAPE_TYPE_SPHERE
			),
	shape(
			vec3(0.0, 0.0, 0.0),
			vec4(1.0, 1.0, 2.5, 0.0),
			SHAPE_TYPE_RECT
			)
	);*/

vec2 smin( float a, float b, float k ) { // from https://iquilezles.org/articles/smin/
	float h = 1.0 - min( abs(a-b)/(6.0*k), 1.0 );
	float w = h*h*h;
	float m = w*0.5;
	float s = w*k; 
	return (a<b) ? vec2(a-s,m) : vec2(b-s,1.0-m);
}

vec2 smax( float a, float b, float k ) { // from https://iquilezles.org/articles/smin/
	vec2 imin = smin(-a, -b, k);
	return vec2(-imin.x, imin.y);
}

float distToScene(vec3 position) {
	//return 101.0;
	//sphere shape = sphere(uPosition.xyz, uRadius);
	/*float distToSphere = getSphereDistance(position, shape);
	float distToRect = getRectDistance(position, cube);
	return min(smax(distToRect, -distToSphere, 0.2).x, distToSphere);*/
	/*float dist = 1000000;
	for(int i = 0; i < shapeCount; i++) {
		dist = smax(dist, -getShapeDistance(position, shapes[i]), 0.5).x;
	}*/
	float sphereDist = getShapeDistance(position, shapes[1]);
	float rectDist = getShapeDistance(position, shapes[0]);
	//float dist = smax(sphereDist, -rectDist, 0.5).x;
	float dist = min(smax(rectDist, -sphereDist, 0.3).x, sphereDist);
	//float dist1 = getShapeDistance(position, shapes[0]);
	//float dist2 = getShapeDistance(position, shapes[1]);
	//return min(dist1, smax(-dist1, dist2, 0.2).x);
	return dist;
}

vec3 sceneCol(vec3 position) {
	/*float sphereDist = getShapeDistance(position, shapes[0]);
	float rectDist = getShapeDistance(position, shapes[1]);

	float smoothPart = smax(rectDist, -sphereDist, 0.2).x;
	float smoothFactor = smax(rectDist, -sphereDist, 0.2).y;

	if(sphereDist < smoothPart) {
		return vec3(0.2, 0.2, 0.2);
	};
	return mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), smoothFactor);*/
	return vec3(1.0, 0.0, 0.0);
}


vec3 getNormal(float distPosX, float distNegX, float distPosY, float distNegY, float distPosZ, float distNegZ) {
	return normalize(vec3(distPosX-distNegX, distPosY-distNegY, distPosZ-distNegZ));
}

void main() {
	//FragColor = vec4(float(shapeCount)/10.0, 0.0, 0.0, 1.0);
	//return;
	float aspectRatio = uResolution.x / uResolution.y;
	camRay ray;
	ray.position = uCamPosition;
	ray.direction = normalize((uCamRotationMatrix * vec4((gl_FragCoord.xy / uResolution * 2.0 - 1.0) * vec2(aspectRatio, 1.0), 1.0, 0.0)).xyz);

	int max_steps = 100;
	bool hit = false;
	vec3 normal = vec3(0);
	float t = 0.5;
	int step_count = 0;
	for(; true; step_count++) {
		//if(step_count > 500) break;
		float dist = distToScene(ray.position);

		if(dist < 0.001) {
			hit = true;

			float step = 0.001;
			float distToShapePosX = distToScene(ray.position + vec3(step, 0.0, 0.0));
			float distToShapePosY = distToScene(ray.position + vec3(0.0, step, 0.0));
			float distToShapePosZ = distToScene(ray.position + vec3(0.0, 0.0, step));

			float distToShapeNegX = distToScene(ray.position - vec3(step, 0.0, 0.0));
			float distToShapeNegY = distToScene(ray.position - vec3(0.0, step, 0.0));
			float distToShapeNegZ = distToScene(ray.position - vec3(0.0, 0.0, step));


			normal = getNormal(distToShapePosX, distToShapeNegX, distToShapePosY, distToShapeNegY, distToShapePosZ, distToShapeNegZ);
			//t = distToShapeFactor;
			break;
		}
		if(dist > 100) {
			break;
		}

		ray.position += ray.direction * dist;
	}


	vec3 lightDir = normalize(vec3(-0.5, 1, -0.3));
	if(hit) {
		float lightness = dot(normal, lightDir) * 0.5 + 0.5;
		vec3 col = sceneCol(ray.position);
		vec3 viewDir = normalize(uCamPosition - ray.position);

		vec3 halfDir = normalize(viewDir + lightDir);

		float shininess = 100;
		float specularComponent = pow(max(dot(normal, halfDir), 0.0), shininess);

		FragColor = vec4(lightness * col + vec3(specularComponent), 1.0);
	} else {
		vec3 sun = vec3(pow(dot(ray.direction, lightDir) * 0.5 + 0.5, 100));
		vec3 sky = vec3(0.5, 0.5, 0.9);
		FragColor = vec4(sun + sky, 1.0);
	}
}
