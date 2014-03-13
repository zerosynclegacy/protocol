# Generate gsl code
XML_SOURCE_CODE = \
	zsync_credit_msg.xml \
	zsync_ftm_msg.xml

CODE_DEPENDENCY = $(XML_SOURCE_CODE:%.xml=%)

# Make executes each line of a recipe in a seperate shell.
# Hence we need to export and generate in one line.
code: $(CODE_DEPENDENCY)

$(CODE_DEPENDENCY):
	export PATH=@bindir@/zproto:$$PATH; gsl -q -script:zproto_c $@;

