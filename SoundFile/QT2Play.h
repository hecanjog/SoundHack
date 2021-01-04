typedef struct
{
	Movie theMovie;
	MovieController theMC;
	Boolean active;
}	QTPlayInfo;


Boolean	StartQTPlay(SoundInfo *theSI, double startTime, double endTime);
float	GetQTPlayTime();
void	StopQTPlay(Boolean wait);

