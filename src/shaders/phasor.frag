#version 450
layout( location = 0 ) in vec2 uv;

layout( location = 0 ) out vec4 color;

#define M_PI 3.14159265358979323846

layout( set = 0, binding = 0 ) uniform UniformBuffer {
	float _f;
	float _b;
	float _o;
	int _impPerKernel;
	int _profile;
	float _pwmRatio;
};

float _kr;
int _seed = 1;
//phasor noise parameters



///////////////////////////////////////////////
//prng
///////////////////////////////////////////////

int N = 15487469;
int x_;
void seed(int s){x_ = s;}
int next() { x_ *= 3039177861; x_ = x_ % N;return x_; }
float uni_0_1() {return  float(next()) / float(N);}
float uni(float min, float max){ return min + (uni_0_1() * (max - min));}


int morton(int x, int y)
{
  int z = 0;
  for (int i = 0 ; i < 32* 4 ; i++) {
    z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
  }
  return z;
}

void init_noise()
{
    _kr = sqrt(-log(0.05) / M_PI) / _b;
}


vec2 phasor_kernel(vec2 x, float f, float b, float o, float phi)
{
    float a = exp(-M_PI * (b * b) * ((x.x * x.x) + (x.y * x.y)));
    float s = sin (2.0* M_PI * f  * (x.x*cos(o) + x.y*sin(o))+phi);
    float c = cos (2.0* M_PI * f  * (x.x*cos(o) + x.y*sin(o))+phi);
    return vec2(a*c,a*s);
}



vec2 phasor_cell(ivec2 ij, vec2 uv)
{
	int s= morton(ij.x,ij.y) + 333;
	s = s==0? 1: s +_seed;
	seed(s);
	int impulse  =0;
	int nImpulse = _impPerKernel;
	float  cellsz = 2.0 * _kr;
	vec2 noise = vec2(0.0);
	while (impulse <= nImpulse){
		vec2 impulse_centre = vec2(uni_0_1(),uni_0_1());
		vec2 d = (uv - impulse_centre) *cellsz;
		float rp = uni(0.0,2.0*M_PI) ;
        vec2 trueUv = ((vec2(ij) + impulse_centre) *cellsz);
		trueUv.y = -trueUv.y;
		noise += phasor_kernel(d, _f, _b ,_o,rp );
		impulse++;
	}
	return noise;
}

vec2 phasor_noise(vec2 uv)
{   
	float cellsz = 2.0 *_kr;
	vec2 _ij = uv / cellsz;
	ivec2  ij = ivec2(_ij);
	vec2  fij = _ij - vec2(ij);
	vec2 noise = vec2(0.0);
	for (int j = -2; j <= 2; j++) {
		for (int i = -2; i <= 2; i++) {
			ivec2 nij = ivec2(i, j);
			noise += phasor_cell(ij + nij , fij - vec2(nij));
		}
	}
    return noise;
}



void main(){
	init_noise();
	vec2 noise = phasor_noise(uv);
	if(_profile == 0){ // complex view
		color = vec4(noise, 0.0,0.0);
	}
	else if(_profile == 1){// normalised complex view
		color = vec4(normalize(noise), 0.0,0.0);
	}
	else if(_profile == 2){// sinewave
		float phi = atan(noise.y,noise.x);
		color = vec4(vec3(sin(phi)*0.5 + 0.5),0.0);
	}
	else if(_profile == 3){// sawtooth
		float phi = atan(noise.y,noise.x) + M_PI;
		color = vec4(vec3(phi/(2.0 * M_PI)),0.0);
	}
	else if(_profile == 4){
		float phi = atan(noise.y,noise.x) + M_PI;
		color = vec4(vec3(phi < _pwmRatio * 2.0 * M_PI ? 1.0 : 0.0),0.0);
	}
}
