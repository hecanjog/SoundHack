/*Function prototypes for Windows.c*/
void	GetWindow(float window[], long size, long type);
void	HammingWindow(float window[], long size);
void	VonHannWindow(float window[], long size);
void	RampWindow(float window[], long size);
void	RectangleWindow(float window[], long size);
void	SincWindow(float window[], long size);
void	KaiserWindow(float window[], long size);
float	ino(float x);
void	TriangleWindow(float window[], long size);

