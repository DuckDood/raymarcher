#version 330 core

out vec4 FragColor;

uniform vec2 uResolution;

uniform mat4 uCamRotationMatrix;
uniform vec3 uCamPosition;

struct sphere {
	vec3 position;
	float radius;
};


struct camRay {
	vec3 direction;
	vec3 position;
};

float getSphereDistance(camRay ray, sphere shape) {
	return length(ray.position - shape.position) - shape.radius;
}

sphere shape = sphere(
		vec3(0.0, 0.0, 3.0),
		2.0
		);

void main() {
	vec4 aspectRatio = vec4(uResolution.x / uResolution.y, 1.0, 1.0, 1.0);
	camRay ray;
	ray.position = uCamPosition;
	ray.direction = normalize((uCamRotationMatrix * vec4(gl_FragCoord.xy / uResolution * 2.0 - 1.0, 1.0, 0.0) * aspectRatio).xyz);

	int max_steps = 100;
	bool hit = false;
	for(int i = 0; i < 100; i++) {
		float distToShape = getSphereDistance(ray, shape);

		if(distToShape < 0.01) {
			hit = true;
			break;
		}

		ray.position += ray.direction * distToShape;
	}

	if(hit) {
		FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	} else {
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
}
