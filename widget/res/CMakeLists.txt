# install edj
ADD_CUSTOM_TARGET(gallerywidget.edj
		COMMAND edje_cc  ${EDJE_CFLAGS}
		${CMAKE_CURRENT_SOURCE_DIR}/gallerywidget.edc gallerywidget.edj
		-id ${CMAKE_CURRENT_SOURCE_DIR}/images/
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gallerywidget.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} gallerywidget.edj)
INSTALL(FILES gallerywidget.edj DESTINATION ${EDJDIR})
