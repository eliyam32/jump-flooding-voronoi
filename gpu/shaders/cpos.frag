varying vec4 color;

void main()
{
	gl_FragData[0] = vec4( gl_FragCoord.st, 0.0, 1.0 );
	gl_FragData[1] = color;
}
