#ifndef SOURCES_PROJECT_CONF_PROJECT_CONF_H_
#define SOURCES_PROJECT_CONF_PROJECT_CONF_H_
/**
 * @file project_conf.h
 * @author Mykhaylo Shcherbak
 * @e mikl74@yahoo.com
 * @date 21-11-2019
 * @version 1.00
 * @brief Contains configuration constants for the project
 */
enum
{
	NLEDS 	= 			144,	/**< Number of leds in the strip */
	T1MIN 	= 			10, 	/**< Minimum time from going out of pitlane to finish line */
    T1MIN2 	= 			10, 	/**< Minimum time from turning the stick on and going out from the pitlane */
    T2MIN 	=   		7,  	/**< Minimum time from intermediate point to crossing finish line */
    T2MIN2 	=   		4,  	/**< Minimum time from going out the pitlane to the intermediate point */
	TSEQ_MIN= 			30, 	/**< Minimum lap time */
	TSEQ_MAX=   		240,	/**< Maximum lap time */
	TLIGHT_MIN= 		1,		/**< Min tlight random time (0.1s) */
	TLIGHT_MAX= 		100,	/**< Max tlight random time (10s) */
	PODNOS_MODE_MIN =   1,      /**< Min length of stop-and-go  mode in seconds */
	PODNOS_MODE_MAX =   50,     /**< Max length of stop-and-go  mode in seconds */
	MAX_BRIGHNESS_LEVELS = 4 /**< Number of brightness levels */
};

#endif /* SOURCES_PROJECT_CONF_PROJECT_CONF_H_ */
