#ifndef PDFH
#define PDFH

class pdf  {
	public:
		virtual float value(const vec3& direction) const = 0;
		virtual vec3 generate() const = 0;
};

#endif
