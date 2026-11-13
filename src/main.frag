#version 330 core

out vec4 FragColor;

uniform float uTime;
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

/*vec3 getNormal(float distCenter, float distPosX, float distPosY, float distPosZ) {
	return normalize(vec3(distPosX-distCenter, distPosY-distCenter, distPosZ-distCenter));
}*/

sphere shape = sphere(
		vec3(0.0, cos(uTime), 0.0),
		1.0
		);

rect cube = rect(
			vec3(0.0, 0.0, sin(uTime*2)*0),
			vec3(1.0, 1.0, 2.5)
		);


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
	float distToSphere = getSphereDistance(position, shape);
	float distToRect = getRectDistance(position, cube);
	return min(smax(distToRect, -distToSphere, 0.2).x, distToSphere);
}

vec3 sceneCol(vec3 position) {
	float distToSphere = getSphereDistance(position, shape);
	float distToRect = getRectDistance(position, cube);
	float smoothPart = smax(distToRect, -distToSphere, 0.2).x;
	float smoothFactor = smax(distToRect, -distToSphere, 0.2).y;
	float spherePart = distToSphere;
	if(spherePart < smoothPart) {
		return vec3(0.2, 0.2, 0.2);
	};
	return mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), smoothFactor);
}


vec3 getNormal(float distPosX, float distNegX, float distPosY, float distNegY, float distPosZ, float distNegZ) {
	return normalize(vec3(distPosX-distNegX, distPosY-distNegY, distPosZ-distNegZ));
}

void main() {
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
