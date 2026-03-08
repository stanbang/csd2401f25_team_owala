#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    float alpha = texture(uTex, vUV).r;
    FragColor = vec4(uColor.rgb, uColor.a * alpha);
}