#version 330 core

out vec4 FragColor;

uniform vec2 resolution;

uniform mat4 camRotationMatrix;

void main() {
	vec3 viewDir = normalize((camRotationMatrix * vec4(gl_FragCoord.xy / resolution * 2.0 - 1.0, 1.0, 0.0)).xyz);
	FragColor = vec4(viewDir, 1.0);
}
