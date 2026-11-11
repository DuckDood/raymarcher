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

vec3 getSphereNormal(vec3 position, sphere shape) {
	return normalize(position - shape.position);
}

sphere shape = sphere(
		vec3(0.0, 0.0, 3.0),
		2.0
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
		float distToShape = getSphereDistance(ray, shape);

		if(distToShape < 0.01) {
			hit = true;
			normal = getSphereNormal(ray.position, shape);
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
