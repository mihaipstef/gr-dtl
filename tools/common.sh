
function to_jq_args() {
	jq_args=""
	for arg in "$@"
	do
		if [[ $arg == *"="* ]]; then
			config_key=$(echo $arg | cut -f1 -d=)
			config_val=$(echo $arg | cut -f2 -d=)
			if [[ -z "$jq_args" ]]; then
				jq_args="$jq_args .$config_key=$config_val"
			else
				jq_args="$jq_args | .$config_key=$config_val"
			fi
		fi
	done
    echo "$jq_args"
}