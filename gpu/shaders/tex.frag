#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect tex; /* the texture to read from */

void main()
{
	gl_FragColor = texture2DRect( tex, gl_FragCoord.st - vec2(0.5) );
}
