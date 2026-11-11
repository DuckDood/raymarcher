#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;

uniform mat4 uCamRotationMatrix;
uniform vec3 uCamPosition;

struct sphere {
	vec3 position;
	float radius;
};

struct rect {
	vec3 position;
	vec3 size;
};


struct camRay {
	vec3 direction;
	vec3 position;
};

float getSphereDistance(vec3 ray, sphere shape) {
	return length(ray - shape.position) - shape.radius;
}

float getRectDistance(vec3 ray, rect shape) {
	// general implementation from sebastian lague's video on ray marching
	vec3 off = abs(ray - shape.position) - shape.size;

	float unsignedDist = length(max(off, 0.0));

	float dstInsideBox = max(max(min(off.x,0.0),min(off.y,0.0)),min(off.z,0.0));

	return unsignedDist + dstInsideBox;
}

vec3 getNormal(float distCenter, float distPosX, float distPosY, float distPosZ) {
	return normalize(vec3(distPosX-distCenter, distPosY-distCenter, distPosZ-distCenter));
}

sphere shape = sphere(
		vec3(0.0, 0.0, 3.0),
		2.0
		);

rect cube = rect(
			vec3(0.0, 0.0, 5.0),
			vec3(1.0, 1.0, 2.0)
		);

void main() {
	float aspectRatio = uResolution.x / uResolution.y;
	camRay ray;
	ray.position = uCamPosition;
	ray.direction = normalize((uCamRotationMatrix * vec4((gl_FragCoord.xy / uResolution * 2.0 - 1.0) * vec2(aspectRatio, 1.0), 1.0, 0.0)).xyz);

	int max_steps = 100;
	bool hit = false;
	vec3 normal = vec3(0);
	for(int i = 0; i < 100; i++) {
		float distToSphere = getSphereDistance(ray.position, shape);
		float distToRect = getRectDistance(ray.position, cube);

		float distToShape = max(distToSphere, -distToRect);

		if(distToShape < 0.01) {
			hit = true;

			float step = 0.001;
			distToSphere = getSphereDistance(ray.position + vec3(step, 0.0, 0.0), shape);
			distToRect = getRectDistance(ray.position + vec3(step, 0.0, 0.0), cube);

			float distToShapeX = max(distToSphere, -distToRect);

			distToSphere = getSphereDistance(ray.position + vec3(0.0, step, 0.0), shape);
			distToRect = getRectDistance(ray.position + vec3(0.0, step, 0.0), cube);

			float distToShapeY = max(distToSphere, -distToRect);

			distToSphere = getSphereDistance(ray.position + vec3(0.0, 0.0, step), shape);
			distToRect = getRectDistance(ray.position + vec3(0.0, 0.0, step), cube);

			float distToShapeZ = max(distToSphere, -distToRect);

			normal = getNormal(distToShape, distToShapeX, distToShapeY, distToShapeZ);
			break;
		}

		ray.position += ray.direction * distToShape;
	}

	if(hit) {
		float lightness = dot(normal, normalize(vec3(-0.5, 1, -0.3))) * 0.5 + 0.5;
		FragColor = vec4(lightness * 1.0, 0.0, 0.0, 1.0);
	} else {
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
