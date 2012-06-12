#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect tex0,tex1,tex2,tex3; /* buffers */
uniform float width,height; /* window dimensions */
uniform int texId; /* which texture to use */

void main()
{
	/*if( texId == 0 )
		gl_FragColor = texture2DRect( tex0, gl_FragCoord.st - vec2(0.5) ) / vec4( width, height, 1.0, 1.0 );
	else if( texId == 1 )
		gl_FragColor = texture2DRect( tex2, gl_FragCoord.st - vec2(0.5) ) / vec4( width, height, 1.0, 1.0 );*/

	if( texId == 0 )
		gl_FragColor = texture2DRect( tex1, gl_FragCoord.st - vec2(0.5) );
	else if( texId == 1 )
		gl_FragColor = texture2DRect( tex3, gl_FragCoord.st - vec2(0.5) );

	/*if( texId == 0 )
		gl_FragColor = texture2DRect( tex1, gl_FragCoord.st - vec2(0.5) ) / vec4( width, height, 1.0, 1.0 );
	else if( texId == 1 )
		gl_FragColor = texture2DRect( tex3, gl_FragCoord.st - vec2(0.5) ) / vec4( width, height, 1.0, 1.0 );*/
}
